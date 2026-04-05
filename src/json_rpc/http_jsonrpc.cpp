#include "http_jsonrpc.h"
#include "json_serialization.h"
#include "logger.h"
#include "config.h"
#include "auth.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace mcp {
    class HttpJsonRpcServer::Impl {
    public:
        httplib::Server server;

        Impl() {
            server.set_error_handler([](const httplib::Request& req, httplib::Response& resp) {
                json err_response = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", jsonrpc_errc::InternalError},
                        {"message", "Internal server error"}
                    }},
                    {"id", nullptr}
                };

                resp.set_content(err_response.dump(), "application/json");
            });
        }
    };

    HttpJsonRpcServer::HttpJsonRpcServer(JsonRpcDispatcher&& dispatcher)
        : HttpJsonRpcServer(std::move(dispatcher), "0.0.0.0", MCP_CONFIG.getServerPort()) {
        MCP_LOG_INFO("HTTP JSON-RPC server initialized from config");
    }

    HttpJsonRpcServer::HttpJsonRpcServer(JsonRpcDispatcher&& dispatcher, const std::string& host, int port):
        dispatcher_(std::move(dispatcher)), host_(host), port_(port), impl_(std::make_unique<Impl>()){
        MCP_LOG_INFO("HTTP JSON-RPC server created on {}:{}", host_, port_);

        impl_->server.Post("/jsonrpc", [this](const httplib::Request& req, httplib::Response& resp) {
            resp.set_header("Access-Control-Allow-Origin", "*");
            resp.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            resp.set_header("Access-Control-Allow-Headers", "Content-Type, X-API-Key");

            // 认证检查
            if (MCP_AUTH.isEnabled()) {
                auto api_key_it = req.headers.find("X-API-Key");
                if (api_key_it == req.headers.end()) {
                    MCP_LOG_WARN("Missing API key in request");
                    json error_response = {
                        {"jsonrpc", "2.0"},
                        {"error", {
                            {"code", 401},
                            {"message", "Missing API key"}
                        }},
                        {"id", nullptr}
                    };
                    resp.set_content(error_response.dump(), "application/json");
                    resp.status = 401;
                    return;
                }
                
                if (!MCP_AUTH.validateApiKey(api_key_it->second)) {
                    MCP_LOG_WARN("Invalid API key: {}", api_key_it->second);
                    json error_response = {
                        {"jsonrpc", "2.0"},
                        {"error", {
                            {"code", 401},
                            {"message", "Invalid API key"}
                        }},
                        {"id", nullptr}
                    };
                    resp.set_content(error_response.dump(), "application/json");
                    resp.status = 401;
                    return;
                }
            }

            try {
                std::string response = handleRequest(req.body);
                resp.set_content(response, "application/json");
                resp.status = 200;
            } catch (const std::exception& e) {
                MCP_LOG_ERROR("Error handing request: {}", e.what());
                json error_response = {
                    {"jsonrpc", "2.0"},
                    {"error", {
                        {"code", jsonrpc_errc::InternalError},
                        {"message", e.what()}
                    }},
                    {"id", nullptr}
                };
                resp.set_content(error_response.dump(), "application/json");
                resp.status = 500;
            }
        });

        impl_->server.Options("/jsonrpc", [](const httplib::Request& req, httplib::Response& resp) {
            resp.set_header("Access-Control-Allow-Origin", "*");
            resp.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
            resp.set_header("Access-Control-Allow-Headers", "Content-Type, X-API-Key");
            resp.status = 204;
        });

        impl_->server.Get("/jsonrpc", [](const httplib::Request& req, httplib::Response& resp) {
            json health = {
                {"status", "OK"},
                {"service", "mcp-json-http"},
                {"timestamp", std::time(nullptr)}
            };
            resp.set_content(health.dump(), "application/json");
        });

        impl_->server.Get("/", [this](const httplib::Request& req, httplib::Response& resp) {
            json info = {
                {"service", "MCP HTTP JSON-RPC Server"},
                {"version", "1.0.0"},
                {"endpoints", {
                    {{"path", "/jsonrpc"}, {"method", "POST"}, {"description", "JSON-RPC 2.0 endpoint"}},
                    {{"path", "/health"}, {"method", "GET"}, {"description", "Health check"}},
                    {{"path", "/sse/events"}, {"method", "GET"}, {"description", "Server status event stream (SSE)"}},
                    {{"path", "/sse/tool_calls"}, {"method", "GET"}, {"description", "Tool call monitoring stream (SSE)"}},
                    {{"path", "/"}, {"method", "GET"}, {"description", "Server information"}}
                }}
            };
            resp.set_content(info.dump(2), "application/json");
        });
    }

    JsonRpcResponse HttpJsonRpcServer::processSingleRequest(const JsonRpcRequest& req) {
        JsonRpcResponse resp;
        resp.json_rpc = "2.0";
        if (req.id.has_value()) {
            resp.id = *req.id;
        }
        try {
            if (!dispatcher_.hasHandler(req.method)) {
                resp.error = {
                    jsonrpc_errc::MethodNotFound,
                    "method not found",
                    std::nullopt
                };
            } else {
                resp.result = dispatcher_.call(req.method, req.params.value_or(json::object()));
            }
        } catch (const std::exception& e) {
            resp.error = {
                jsonrpc_errc::InternalError,
                e.what(),
                std::nullopt
            };
        }
        return resp;
    }

    std::string HttpJsonRpcServer::createErrorResponse(int code, const std::string& message) {
        json error_resp = {
            {"jsonrpc", "2.0"},
            {"error", {
                {"code", code},
                {"message", message}
            }},
            {"id", nullptr}
        };
        return error_resp.dump();
    }

    std::string HttpJsonRpcServer::handleRequest(const std::string& request) {
        MCP_LOG_DEBUG("Handle Request: {}", request);

        try {
            json req_json = json::parse(request);
            if (req_json.is_array()) {
                json batch_resp = json::array();
                for (const auto& item : req_json) {
                    try {
                        JsonRpcRequest req;
                        req.json_rpc = item.value("jsonrpc", "2.0");
                        req.method = item.at("method").get<std::string>();
                        if (item.contains("id")) {
                            req.id = item["id"];
                        }
                        if (item.contains("params")) {
                            req.params = item["params"];
                        }

                        JsonRpcResponse resp = processSingleRequest(req);
                        if (req.id.has_value()) {
                            json resp_json;
                            to_json(resp_json, resp);
                            batch_resp.push_back(resp_json);
                        }
                    } catch (const std::exception& e) {
                        MCP_LOG_ERROR("Error when processing request: {}", e.what());
                        batch_resp.push_back(json::parse(createErrorResponse(jsonrpc_errc::InternalError, e.what())));
                    }
                }

                std::string response = batch_resp.dump();
                MCP_LOG_DEBUG("Batch Response: {}", response);
                return response;
            }

            JsonRpcRequest req = req_json;
            if (!req.id.has_value()) {
                if (dispatcher_.hasHandler(req.method)) {
                    dispatcher_.call(req.method, req.params.value_or(json::object()));
                }
                MCP_LOG_DEBUG("Notification request, no response");
                return "";
            }

            JsonRpcResponse resp = processSingleRequest(req);
            json resp_json;
            to_json(resp_json, resp);
            std::string resp_str = resp_json.dump();
            MCP_LOG_DEBUG("Response: {}", resp_str);
            return resp_str;
        } catch (const json::parse_error& e) {
            MCP_LOG_ERROR("Error in JSON parse: {}", e.what());
            return createErrorResponse(jsonrpc_errc::ParseError, e.what());
        } catch (const std::exception& e) {
            MCP_LOG_ERROR("Error when processing request: {}", e.what());
            return createErrorResponse(jsonrpc_errc::InternalError, e.what());
        }
    }

    void HttpJsonRpcServer::registerSseEndPoint(const std::string& path, sseCallback callback) {
        impl_->server.Get(path, [callback](const httplib::Request& req, httplib::Response& resp) {
            resp.set_header("Content-Type", "text/event_stream");
            resp.set_header("Cache-Control", "no-cache");
            resp.set_header("Connection", "keep-alive");
            resp.set_header("Access-Control-Allow-Origin", "*");
            resp.set_header("Access-Control-Allow-Headers", "X-API-Key");
            resp.set_header("X-Access-Buffering", "no");

            // 认证检查
            if (MCP_AUTH.isEnabled()) {
                auto api_key_it = req.headers.find("X-API-Key");
                if (api_key_it == req.headers.end()) {
                    MCP_LOG_WARN("Missing API key in SSE request");
                    resp.status = 401;
                    resp.set_content("Missing API key", "text/plain");
                    return;
                }
                
                if (!MCP_AUTH.validateApiKey(api_key_it->second)) {
                    MCP_LOG_WARN("Invalid API key for SSE: {}", api_key_it->second);
                    resp.status = 401;
                    resp.set_content("Invalid API key", "text/plain");
                    return;
                }
            }

            resp.set_chunked_content_provider("text/event_stream", [callback](size_t, httplib::DataSink& sink) {
                auto send_event = [&sink](const std::string& data) {
                    std::string event = "data: " + data + "\n\n";
                    sink.write(event.c_str(), event.size());
                };
                try {
                    callback(send_event);
                } catch (const std::exception& e) {
                    MCP_LOG_ERROR("Error in SSE Callback: {}", e.what());
                }
                return true;
            });
        });
    }

    void HttpJsonRpcServer::run() {
        if (!running_.exchange(true)) {
            MCP_LOG_WARN("Server is already running");
            return;
        }

        MCP_LOG_INFO("Server is running on {}:{}", host_, port_);
        if (!impl_->server.listen(host_, port_)) {
            running_ = false;
            MCP_LOG_ERROR("Failed to listen to port on {}:{}", host_, port_);
        }
        MCP_LOG_INFO("HTTP JSON-RPC server Stopped");
        impl_->server.stop();
    }

    void HttpJsonRpcServer::stop() {
        if (running_.exchange(false)) {
            MCP_LOG_WARN("Server is already stopped");
            return;
        }
        MCP_LOG_INFO("Stopping HTTP JSON-RPC server...");
        impl_->server.stop();
    }

    HttpJsonRpcServer::~HttpJsonRpcServer() {
        stop();
    }
}

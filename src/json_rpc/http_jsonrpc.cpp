#include "http_jsonrpc.h"
#include "json_serialization.h"
#include "logger.h"
#include "config.h"

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
                        {"code", -32603},
                        {"message", "Internal server error"}
                    }},
                    {"id", nullptr}
                };

                resp.set_content(err_response.dump(), "applocation/json");
            });
        }
    };

    HttpJsonRpcServer::HttpJsonRpcServer(JsonRpcDispatcher dispatcher)
        : HttpJsonRpcServer(std::move(dispatcher), "0.0.0.0", MCP_CONFIG.getServerPort())
}

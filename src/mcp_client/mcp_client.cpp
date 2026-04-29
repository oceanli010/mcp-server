#include "mcp_client.h"
#include <httplib.h>
#include <stdexcept>

#include "logger.h"

namespace mcp {
    //http接口实现
    class HttpTransPort::Impl_ {
    public:
        Impl_(const std::string& host, int port) : host_(host), port_(port), client_(host, port) {
            //客户端超时设置
            client_.set_connection_timeout(5, 0);
            client_.set_read_timeout(30, 0);
        };

        json send(const json& request) {
            auto res = client_.Post("/jsonrpc", request.dump(), "application/json");
            if (!res) {
                MCP_LOG_ERROR("Failed to connect to MCP server at {}:{}", host_, port_);
                throw std::runtime_error("Failed to connect to MCP server at " + host_ + ":" + std::to_string(port_));
            }

            if (res->status != 200) {
                MCP_LOG_ERROR("HTTP error: {}", std::to_string(res->status));
                throw std::runtime_error("HTTP error: " + std::to_string(res->status));
            }

            try {
                return json::parse(res->body);
            } catch (const std::exception& e) {
                MCP_LOG_ERROR("Failed to parse JSON response: {}", e.what());
                throw std::runtime_error("Failed to parse JSON response: " + std::string(e.what()));
            }
        }

    private:
        std::string host_;
        int port_;
        httplib::Client client_;
    };

    HttpTransPort::HttpTransPort(const std::string& host, int port) :impl_(std::make_unique<Impl_>(host, port)) {}

    HttpTransPort::~HttpTransPort() = default;

    json HttpTransPort::send(const json& request) {
        return impl_->send(request);
    }

    McpClient::McpClient(const std::string& host, int port) :transport_(std::make_unique<HttpTransPort>(host, port)) {}

    McpClient::~McpClient() = default;

    json McpClient::send_request(const std::string& method, const json& params) {
        last_error_.clear();

        try {
            //包装请求体
            json request = {
                {"jsonrpc", "2.0"},
                {"method", method},
                {"params", params},
                {"id", ++request_id_}
            };

            //发送调用请求
            json response = transport_->send(request);
            if (response.contains("error")) {
                last_error_ = response["error"]["message"].get<std::string>();
                MCP_LOG_ERROR("Failed to send request: {}", last_error_);
                throw std::runtime_error("MCP Error: " + last_error_);
            }

            return response["result"];
        } catch (const std::exception& e) {
            last_error_ = e.what();
            MCP_LOG_ERROR("Failed to send request: {}", last_error_);
            throw;
        }
    }

    InitializeResult McpClient::initialize() {
        json result = send_request("initialize", json::object());
        return InitializeResult::from_json(result);
    }

    std::vector<Tool> McpClient::list_tools() {
        json result = send_request("tools/list", json::object());
        std::vector<Tool> tools;
        for (const auto& tool : result["tools"]) {
            tools.push_back(Tool::from_json(tool));
        }
        return tools;
    }

    ToolResult McpClient::call_tool(const std::string& name, const json& arguments) {
        json params = {
            {"name", name},
            {"arguments", arguments}
        };

        json result = send_request("tools/call", params);
        return ToolResult::from_json(result);
    }

    std::vector<Resources> McpClient::list_resources() {
        json result = send_request("resources/list", json::object());
        std::vector<Resources> resources;
        for (const auto& resource : result["resources"]) {
            resources.push_back(Resources::from_json(resource));
        }
        return resources;
    }

    ResourcesContent McpClient::read_resources(const std::string& uri) {
        json params = {{"uri", uri}};
        json result = send_request("resources/read", params);
        return ResourcesContent::from_json(result["content_item"][0]);
    }

    std::vector<Prompt> McpClient::list_prompts() {
        json result = send_request("prompts/list", json::object());
        std::vector<Prompt> prompts;
        for (const auto& prompt : result["prompts"]) {
            prompts.push_back(Prompt::from_json(prompt));
        }
        return prompts;
    }

    std::vector<PromptMessage> McpClient::get_prompt(const std::string& name, const json& arguments) {
        json params = {
            {"name", name},
            {"arguments", arguments}
        };
        json result = send_request("prompts/get", params);
        std::vector<PromptMessage> prompt_messages;
        for (const auto& prompt_msg : result["prompts"]) {
            prompt_messages.push_back(PromptMessage::from_json(prompt_msg));
        }
        return prompt_messages;
    }

    std::string McpClient::get_last_error() const {
        return last_error_;
    }
}

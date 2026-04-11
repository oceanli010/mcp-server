#pragma once

#include "types.h"

#include <string>
#include <memory>
#include <vector>

namespace mcp {
    class TransPort {
    public:
        virtual ~TransPort() = default;
        virtual json send(const json& request) = 0;
    };

    class HttpTransPort : public TransPort {
    public:
        HttpTransPort(const std::string& host, int port);
        ~HttpTransPort();

        json send(const json& request) override;

    private:
        class Impl_;
        std::unique_ptr<Impl_> impl_;
    };

    class McpClient {
    public:
        McpClient(const std::string& host, int port);
        ~McpClient();

        InitializeResult initialize();

        std::vector<Tool> list_tools();
        ToolResult call_tool(const std::string& name, const json& arguments);

        std::vector<Resources> list_resources();
        ResourcesContent read_resources(const std::string& uri);

        std::vector<Prompt> list_prompts();
        std::vector<PromptMessage> get_prompt(const std::string& name, const json& arguments);

        std::string get_last_error() const;

        McpClient(const McpClient&) = delete;
        McpClient& operator=(const McpClient&) = delete;

    private:
        json send_request(const std::string& method, const json& params);

        std::unique_ptr<HttpTransPort> transport_;
        int request_id_ = 0;
        std::string last_error_;
    };
}
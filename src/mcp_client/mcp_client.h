///MCP 客户端
///方便其他程序调用 MCP 服务器

#pragma once

#include "types.h"

#include <string>
#include <memory>
#include <vector>

namespace mcp {
    //接口抽象化
    class TransPort {
    public:
        virtual ~TransPort() = default;
        virtual json send(const json& request) = 0;
    };

    //HTTP传输实现
    class HttpTransPort : public TransPort {
    public:
        HttpTransPort(const std::string& host, int port);
        ~HttpTransPort() override;

        //发送POST请求至 /jsonrpc 端点
        json send(const json& request) override;

    private:
        class Impl_;
        std::unique_ptr<Impl_> impl_;
    };

    //MCP客户端实现
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
        //方法调用请求
        json send_request(const std::string& method, const json& params);

        std::unique_ptr<HttpTransPort> transport_;
        int request_id_ = 0;
        std::string last_error_;
    };
}
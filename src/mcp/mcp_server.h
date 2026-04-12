///MCP 服务器核心类，管理工具、资源和提示词

#pragma once

#include "types.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace mcp {
    class McpServer {
    public:
        using ToolHandler = std::function<ToolResult(const json& arguments)>;
        using ResourceProvider = std::function<ResourcesContent(const std::string& uri)>;
        using PromptGenerator = std::function<std::vector<PromptMessage>(const json& arguments)>;
        using SseEventCallback = std::function<void(const json& arguments)>;

        //McpServer构造函数
        McpServer(const std::string& name, const std::string& version);

        //McpServer初始化
        InitializeResult get_initialize_result() const;

        //设置服务器能力
        void set_capabilities(const ServerCapabilities& server_capabilities);

        //工具处理函数（注册、列出、调用、检查）
        void register_tool(const Tool& tool, ToolHandler tool_handler);
        std::vector<Tool> list_tools() const;
        ToolResult call_tool(const std::string& name, const json& arguments);
        bool has_tool(const std::string& name) const;

        //资源处理函数（注册、列出、读取、检查）
        void register_resource(const Resources& resources, ResourceProvider resource_provider);
        std::vector<Resources> list_resources() const;
        ResourcesContent read_resources(const std::string& uri);
        bool has_resource(const std::string& uri) const;

        //提示词处理函数（注册、列出、获取、检查）
        void register_prompt(const Prompt& prompt, PromptGenerator prompt_generator);
        std::vector<Prompt> list_prompts() const;
        std::vector<PromptMessage> get_prompt(const std::string& name, const json& arguments);
        bool has_prompt(const std::string& name) const;

        //SSE回调
        void set_sse_callback(SseEventCallback callback);

    private:
        ServerInfo server_info_;
        ServerCapabilities server_capabilities_;

        std::unordered_map<std::string, Tool> tools_;
        std::unordered_map<std::string, ToolHandler> tool_handlers_;
        mutable std::mutex tools_mutex_;

        std::unordered_map<std::string, Resources> resources_;
        std::unordered_map<std::string, ResourceProvider> resource_providers_;
        mutable std::mutex resource_mutex_;

        std::unordered_map<std::string, Prompt> prompts_;
        std::unordered_map<std::string, PromptGenerator> prompt_generators_;
        mutable std::mutex prompt_mutex_;

        SseEventCallback sse_event_callback_;
        mutable std::mutex sse_mutex_;
    };
}
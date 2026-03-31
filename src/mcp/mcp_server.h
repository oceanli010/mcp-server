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

        McpServer(const std::string& name, const std::string version);

        InitializeResult get_initialize_result() const;

        void set_capabilities(const ServerCapabilities& server_capabilities);

        void register_tool(const Tool& tool, ToolHandler tool_handler);
        std::vector<Tool> list_tools() const;
        ToolResult call_tool(const std::string& name, const json& arguments);
        bool has_tool(const std::string& name) const;

        void register_resource(const Resources& resources, ResourceProvider resource_provider);
        std::vector<Resources> list_resources() const;
        ResourcesContent read_resources(const std::string& uri);
        bool has_resource(const std::string& uri) const;

        void register_prompt(const Prompt& prompt, PromptGenerator prompt_generator);
        std::vector<Prompt> list_prompts() const;
        std::vector<PromptMessage> get_prompt(const std::string& name, const json& arguments);
        bool has_prompt(const std::string& name) const;

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
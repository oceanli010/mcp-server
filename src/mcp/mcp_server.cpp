#include "mcp_server.h"

#include <shared_mutex>
#include <stdexcept>
#include <utility>
#include "logger.h"

namespace mcp {
    McpServer::McpServer(const std::string& name, const std::string& version) {
        server_info_.name = name;
        server_info_.version = version;

        server_capabilities_.tool_capabilities = ServerCapabilities::ToolCapabilities{false};
        server_capabilities_.resources_capabilities = ServerCapabilities::ResourcesCapabilities{false};
        server_capabilities_.prompt_capabilities = ServerCapabilities::PromptCapabilities{false};
    }

    InitializeResult McpServer::get_initialize_result() const {
        InitializeResult result;
        result.version = "2024-11-05";
        result.capabilities = server_capabilities_;
        result.info = server_info_;
        return result;
    }

    void McpServer::set_capabilities(const ServerCapabilities& server_capabilities) {
        server_capabilities_ = server_capabilities;
    }

    void McpServer::register_tool(const Tool& tool, ToolHandler tool_handler) {
        std::lock_guard<std::mutex> lock(tools_mutex_);

        if (tools_.find(tool.name) != tools_.end()) {
            throw std::runtime_error("McpServer::register_tool: already registered");
        }
        tools_[tool.name] = tool;
        tool_handlers_[tool.name] = std::move(tool_handler);
    }

    std::vector<Tool> McpServer::list_tools() const {
        std::lock_guard<std::mutex> lock(tools_mutex_);

        std::vector<Tool> tools;
        tools.reserve(tools_.size());
        for (const auto& [name, tool] : tools_) {
            tools.push_back(tool);
        }

        return tools;
    }

    ToolResult McpServer::call_tool(const std::string& name, const json& arguments) {
        std::lock_guard<std::mutex> lock(tools_mutex_);

        auto it = tool_handlers_.find(name);
        if (it == tool_handlers_.end()) {
            MCP_LOG_ERROR("Calling Tool: {} not found", name);
            throw std::runtime_error("McpServer::call_tool:" + name + " not registered");
        }
        MCP_LOG_INFO("Use Tool: {}", name);
        try {
            ToolResult result = it->second(arguments);
            MCP_LOG_DEBUG("Calling Tool: {} successful", name);
            return result;
        } catch (const std::exception& e) {
            MCP_LOG_ERROR("Calling Tool failed: {}", e.what());
            ToolResult error;
            error.is_error = true;
            error.content_items.push_back(ContentItem{
                .type = "text",
                .text = std::string("Error executing tool: ") + std::string(e.what())
            });

            return error;
        }
    }

    bool McpServer::has_tool(const std::string& name) const {
        std::lock_guard<std::mutex> lock(tools_mutex_);
        return tools_.find(name) != tools_.end();
    }

    void McpServer::register_resource(const Resources& resources, ResourceProvider resource_provider) {
        std::lock_guard<std::mutex> lock(resource_mutex_);

        if (resources_.find(resources.name) != resources_.end()) {
            MCP_LOG_ERROR("Resource already registered: {}", resources.name);
            throw std::runtime_error("Resource already registered");
        }

        resources_[resources.uri] = resources;
        resource_providers_[resources.uri] = std::move(resource_provider);
    }

    std::vector<Resources> McpServer::list_resources() const {
        std::lock_guard<std::mutex> lock(resource_mutex_);

        std::vector<Resources> result;
        result.reserve(resources_.size());
        for (const auto& [uri, resource] : resources_) {
            result.push_back(resource);
        }

        return result;
    }

    ResourcesContent McpServer::read_resources(const std::string& uri) {
        std::lock_guard<std::mutex> lock(resource_mutex_);

        auto it = resource_providers_.find(uri);
        if (it == resource_providers_.end()) {
            MCP_LOG_ERROR("Resource not found: {}", uri);
            throw std::runtime_error("Resource not found");
        }
        MCP_LOG_INFO("Read resources: {}", uri);

        return it->second(uri);
    }

    bool McpServer::has_resource(const std::string& name) const {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        return resources_.find(name) != resources_.end();
    }

    void McpServer::register_prompt(const Prompt& prompt, PromptGenerator prompt_generator) {
        std::lock_guard<std::mutex> lock(prompt_mutex_);

        if (prompts_.find(prompt.name) != prompts_.end()) {
            MCP_LOG_ERROR("Prompt already registered: {}", prompt.name);
            throw std::runtime_error("Prompt already registered");
        }
        prompts_[prompt.name] = prompt;
        prompt_generators_[prompt.name] = std::move(prompt_generator);
    }

    std::vector<Prompt> McpServer::list_prompts() const {
        std::lock_guard<std::mutex> lock(prompt_mutex_);

        std::vector<Prompt> result;
        result.reserve(prompts_.size());
        for (const auto& [name, prompt] : prompts_) {
            result.push_back(prompt);
        }

        return result;
    }

    std::vector<PromptMessage> McpServer::get_prompt(const std::string& name, const json& arguments) {
        std::lock_guard<std::mutex> lock(prompt_mutex_);

        auto it = prompt_generators_.find(name);
        if (it == prompt_generators_.end()) {
            MCP_LOG_ERROR("Prompt not found: {}", name);
        }
        MCP_LOG_INFO("Prompt found: {}", name);

        return it->second(arguments);
    }

    bool McpServer::has_prompt(const std::string& name) const {
        std::lock_guard<std::mutex> lock(prompt_mutex_);
        return prompts_.find(name) != prompts_.end();
    }

    void McpServer::set_sse_callback(SseEventCallback callback) {
        std::lock_guard<std::mutex> lock(sse_mutex_);
        sse_event_callback_ = std::move(callback);
    }
}

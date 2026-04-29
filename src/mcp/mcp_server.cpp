#include "mcp_server.h"
#include "repository.h"

#include <shared_mutex>
#include <stdexcept>
#include <utility>
#include "logger.h"

namespace mcp {
    McpServer::McpServer(const std::string& name, const std::string& version) {
        server_info_.name = name;server_info_.version = version;

        server_capabilities_.tool_capabilities = ServerCapabilities::ToolCapabilities{false};
        server_capabilities_.resources_capabilities = ServerCapabilities::ResourcesCapabilities{false};
        server_capabilities_.prompt_capabilities = ServerCapabilities::PromptCapabilities{false};
    }

    void McpServer::init_database(std::unique_ptr<McpRepository> repository) {
        repository_ = std::move(repository);
        
        try {
            auto tools = repository_->getAllTools();
            for (const auto& tool : tools) {
                tools_[tool.name] = tool;
            }
            MCP_LOG_INFO("Loaded {} tools from database", tools.size());
        } catch (const std::exception& e) {
            MCP_LOG_WARN("Failed to load tools from database: {}", e.what());
        }

        try {
            auto resources = repository_->getAllResources();
            for (const auto& resource : resources) {
                resources_[resource.uri] = resource;
            }
            MCP_LOG_INFO("Loaded {} resources from database", resources.size());
        } catch (const std::exception& e) {
            MCP_LOG_WARN("Failed to load resources from database: {}", e.what());
        }

        try {
            auto prompts = repository_->getAllPrompts();
            for (const auto& prompt : prompts) {
                prompts_[prompt.name] = prompt;
            }
            MCP_LOG_INFO("Loaded {} prompts from database", prompts.size());
        } catch (const std::exception& e) {
            MCP_LOG_WARN("Failed to load prompts from database: {}", e.what());
        }
    }

    InitializeResult McpServer::get_initialize_result() const {
        InitializeResult result;
        result.version = "2026-04-12";
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
            MCP_LOG_WARN("Tool already registered, updating: {}", tool.name);
        }
        tools_[tool.name] = tool;
        tool_handlers_[tool.name] = std::move(tool_handler);

        if (repository_) {
            try {
                repository_->saveTool(tool);
                MCP_LOG_DEBUG("Saved tool to database: {}", tool.name);
            } catch (const std::exception& e) {
                MCP_LOG_ERROR("Failed to save tool to database: {}", e.what());
            }
        }
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

        if (resources_.find(resources.uri) != resources_.end()) {
            MCP_LOG_WARN("Resource already registered, updating: {}", resources.uri);
        }

        resources_[resources.uri] = resources;
        resource_providers_[resources.uri] = std::move(resource_provider);

        if (repository_) {
            try {
                repository_->saveResource(resources);
                MCP_LOG_DEBUG("Saved resource to database: {}", resources.uri);
            } catch (const std::exception& e) {
                MCP_LOG_ERROR("Failed to save resource to database: {}", e.what());
            }
        }
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

    bool McpServer::has_resource(const std::string& uri) const {
        std::lock_guard<std::mutex> lock(resource_mutex_);
        return resources_.find(uri) != resources_.end();
    }

    void McpServer::register_prompt(const Prompt& prompt, PromptGenerator prompt_generator) {
        std::lock_guard<std::mutex> lock(prompt_mutex_);

        if (prompts_.find(prompt.name) != prompts_.end()) {
            MCP_LOG_WARN("Prompt already registered, updating: {}", prompt.name);
        }
        prompts_[prompt.name] = prompt;
        prompt_generators_[prompt.name] = std::move(prompt_generator);

        if (repository_) {
            try {
                repository_->savePrompt(prompt, {});
                MCP_LOG_DEBUG("Saved prompt to database: {}", prompt.name);
            } catch (const std::exception& e) {
                MCP_LOG_ERROR("Failed to save prompt to database: {}", e.what());
            }
        }
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
            throw std::runtime_error("McpServer::get_prompt:" + name + " not registered");
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

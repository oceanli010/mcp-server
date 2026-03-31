#include "mcp_server.h"
#include <stdexcept>
#include "logger.h"

namespace mcp {
    McpServer::McpServer(const std::string& name, const std::string version) {
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
}

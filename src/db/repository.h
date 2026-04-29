#pragma once

#include "database.h"
#include "dao/tool_dao.h"
#include "dao/resource_dao.h"
#include "dao/prompt_dao.h"
#include "../mcp/types.h"

namespace mcp {
    class McpRepository {
    public:
        explicit McpRepository(SQLite::Database& db);
        
        void saveTool(const Tool& tool);
        Tool getTool(const std::string& name);
        std::vector<Tool> getAllTools();
        void deleteTool(const std::string& name);
        
        void saveResource(const Resources& resource);
        Resources getResource(const std::string& uri);
        std::vector<Resources> getAllResources();
        void deleteResource(const std::string& uri);
        
        void savePrompt(const Prompt& prompt, const std::vector<PromptMessage>& messages);
        Prompt getPrompt(const std::string& name);
        std::vector<Prompt> getAllPrompts();
        void deletePrompt(const std::string& name);
        
    private:
        SQLite::Database& db_;
        ToolDao toolDao_;
        ResourceDao resourceDao_;
        PromptDao promptDao_;
        PromptMessageDao promptMessageDao_;
    };
}

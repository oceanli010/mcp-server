#include "repository.h"
#include <nlohmann/json.hpp>

namespace mcp {
    using json = nlohmann::json;

    McpRepository::McpRepository(SQLite::Database& db) :
        db_(db),
        toolDao_(db),
        resourceDao_(db),
        promptDao_(db),
        promptMessageDao_(db) {}

    void McpRepository::saveTool(const Tool& tool) {
        if (toolDao_.exists(tool.name)) {
            SQLite::Statement getIdQuery(db_, "SELECT id FROM tools WHERE name = ?;");
            getIdQuery.bind(1, tool.name);
            if (getIdQuery.executeStep()) {
                int id = getIdQuery.getColumn(0).getInt();
                toolDao_.update(id, tool);
            }
        } else {
            toolDao_.create(tool);
        }
    }

    Tool McpRepository::getTool(const std::string& name) {
        return toolDao_.getByName(name);
    }

    std::vector<Tool> McpRepository::getAllTools() {
        return toolDao_.list();
    }

    void McpRepository::deleteTool(const std::string& name) {
        if (toolDao_.exists(name)) {
            SQLite::Statement getIdQuery(db_, "SELECT id FROM tools WHERE name = ?;");
            getIdQuery.bind(1, name);
            if (getIdQuery.executeStep()) {
                int id = getIdQuery.getColumn(0).getInt();
                toolDao_.deleteById(id);
            }
        }
    }

    void McpRepository::saveResource(const Resources& resource) {
        if (resourceDao_.exists(resource.uri)) {
            SQLite::Statement getIdQuery(db_, "SELECT id FROM resources WHERE uri = ?;");
            getIdQuery.bind(1, resource.uri);
            if (getIdQuery.executeStep()) {
                int id = getIdQuery.getColumn(0).getInt();
                resourceDao_.update(id, resource);
            }
        } else {
            resourceDao_.create(resource);
        }
    }

    Resources McpRepository::getResource(const std::string& uri) {
        return resourceDao_.getByUri(uri);
    }

    std::vector<Resources> McpRepository::getAllResources() {
        return resourceDao_.list();
    }

    void McpRepository::deleteResource(const std::string& uri) {
        if (resourceDao_.exists(uri)) {
            SQLite::Statement getIdQuery(db_, "SELECT id FROM resources WHERE uri = ?;");
            getIdQuery.bind(1, uri);
            if (getIdQuery.executeStep()) {
                int id = getIdQuery.getColumn(0).getInt();
                resourceDao_.deleteById(id);
            }
        }
    }

    void McpRepository::savePrompt(const Prompt& prompt, const std::vector<PromptMessage>& messages) {
        if (promptDao_.exists(prompt.name)) {
            SQLite::Statement getIdQuery(db_, "SELECT id FROM prompts WHERE name = ?;");
            getIdQuery.bind(1, prompt.name);
            if (getIdQuery.executeStep()) {
                int id = getIdQuery.getColumn(0).getInt();
                promptMessageDao_.deleteByPromptId(id);
                promptDao_.update(id, prompt);
                for (size_t i = 0; i < messages.size(); ++i) {
                    promptMessageDao_.create(id, messages[i], static_cast<int>(i));
                }
            }
        } else {
            int promptId = promptDao_.create(prompt);
            for (size_t i = 0; i < messages.size(); ++i) {
                promptMessageDao_.create(promptId, messages[i], static_cast<int>(i));
            }
        }
    }

    Prompt McpRepository::getPrompt(const std::string& name) {
        return promptDao_.getByName(name);
    }

    std::vector<Prompt> McpRepository::getAllPrompts() {
        return promptDao_.list();
    }

    void McpRepository::deletePrompt(const std::string& name) {
        if (promptDao_.exists(name)) {
            SQLite::Statement getIdQuery(db_, "SELECT id FROM prompts WHERE name = ?;");
            getIdQuery.bind(1, name);
            if (getIdQuery.executeStep()) {
                int id = getIdQuery.getColumn(0).getInt();
                promptMessageDao_.deleteByPromptId(id);
                promptDao_.deleteById(id);
            }
        }
    }
}

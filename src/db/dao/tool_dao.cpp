#include "tool_dao.h"
#include <nlohmann/json.hpp>

namespace mcp {
    using json = nlohmann::json;

    ToolDao::ToolDao(SQLite::Database& db) : db_(db) {}

    int ToolDao::create(const Tool& tool) {
        SQLite::Statement query(db_, "INSERT INTO tools (name, description, input_schema) VALUES (?, ?, ?);");
        query.bind(1, tool.name);
        query.bind(2, tool.description);
        query.bind(3, tool.input_schema.to_json().dump());
        query.exec();
        return static_cast<int>(db_.getLastInsertRowid());
    }

    Tool ToolDao::getById(int id) {
        SQLite::Statement query(db_, "SELECT name, description, input_schema FROM tools WHERE id = ?;");
        query.bind(1, id);
        
        if (query.executeStep()) {
            Tool tool;
            tool.name = query.getColumn(0).getString();
            tool.description = query.getColumn(1).getString();
            tool.input_schema = ToolInputSchema::from_json(json::parse(query.getColumn(2).getString()));
            return tool;
        }
        
        throw std::runtime_error("Tool not found with id: " + std::to_string(id));
    }

    Tool ToolDao::getByName(const std::string& name) {
        SQLite::Statement query(db_, "SELECT id, name, description, input_schema FROM tools WHERE name = ?;");
        query.bind(1, name);
        
        if (query.executeStep()) {
            Tool tool;
            tool.name = query.getColumn(1).getString();
            tool.description = query.getColumn(2).getString();
            tool.input_schema = ToolInputSchema::from_json(json::parse(query.getColumn(3).getString()));
            return tool;
        }
        
        throw std::runtime_error("Tool not found with name: " + name);
    }

    std::vector<Tool> ToolDao::list() {
        std::vector<Tool> tools;
        SQLite::Statement query(db_, "SELECT name, description, input_schema FROM tools;");
        
        while (query.executeStep()) {
            Tool tool;
            tool.name = query.getColumn(0).getString();
            tool.description = query.getColumn(1).getString();
            tool.input_schema = ToolInputSchema::from_json(json::parse(query.getColumn(2).getString()));
            tools.push_back(std::move(tool));
        }
        
        return tools;
    }

    void ToolDao::update(int id, const Tool& tool) {
        SQLite::Statement query(db_, "UPDATE tools SET name = ?, description = ?, input_schema = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;");
        query.bind(1, tool.name);
        query.bind(2, tool.description);
        query.bind(3, tool.input_schema.to_json().dump());
        query.bind(4, id);
        query.exec();
    }

    void ToolDao::deleteById(int id) {
        SQLite::Statement query(db_, "DELETE FROM tools WHERE id = ?;");
        query.bind(1, id);
        query.exec();
    }

    bool ToolDao::exists(const std::string& name) {
        SQLite::Statement query(db_, "SELECT 1 FROM tools WHERE name = ?;");
        query.bind(1, name);
        return query.executeStep();
    }
}

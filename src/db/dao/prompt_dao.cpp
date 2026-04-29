#include "prompt_dao.h"
#include <nlohmann/json.hpp>

#include "logger.h"

namespace mcp {
    using json = nlohmann::json;

    PromptDao::PromptDao(SQLite::Database& db) : db_(db) {}

    int PromptDao::create(const Prompt& prompt) {
        json argsJson = json::array();
        for (const auto& arg : prompt.arguments) {
            argsJson.push_back(arg.to_json());
        }

        //通过参数化查询，防止注入攻击并提高效率
        SQLite::Statement query(db_, "INSERT INTO prompts (name, description, arguments) VALUES (?, ?, ?);");
        query.bind(1, prompt.name);
        query.bind(2, prompt.description.value_or(""));
        query.bind(3, argsJson.dump());
        query.exec();
        return static_cast<int>(db_.getLastInsertRowid());
    }

    Prompt PromptDao::getById(int id) {
        SQLite::Statement query(db_, "SELECT name, description, arguments FROM prompts WHERE id = ?;");
        query.bind(1, id);
        
        if (query.executeStep()) {
            Prompt prompt;
            prompt.name = query.getColumn(0).getString();
            
            std::string descStr = query.getColumn(1).getString();
            if (!descStr.empty()) {
                prompt.description = descStr;
            }
            
            json argsJson = json::parse(query.getColumn(2).getString());
            for (const auto& argJson : argsJson) {
                prompt.arguments.push_back(PromptArgument::from_json(argJson));
            }
            
            return prompt;
        }

        throw std::runtime_error("Prompt not found with id: " + std::to_string(id));
    }

    Prompt PromptDao::getByName(const std::string& name) {
        SQLite::Statement query(db_, "SELECT id, name, description, arguments FROM prompts WHERE name = ?;");
        query.bind(1, name);
        
        if (query.executeStep()) {
            Prompt prompt;
            prompt.name = query.getColumn(1).getString();
            
            std::string descStr = query.getColumn(2).getString();
            if (!descStr.empty()) {
                prompt.description = descStr;
            }
            
            json argsJson = json::parse(query.getColumn(3).getString());
            for (const auto& argJson : argsJson) {
                prompt.arguments.push_back(PromptArgument::from_json(argJson));
            }
            
            return prompt;
        }
        
        throw std::runtime_error("Prompt not found with name: " + name);
    }

    std::vector<Prompt> PromptDao::list() {
        std::vector<Prompt> prompts;
        SQLite::Statement query(db_, "SELECT name, description, arguments FROM prompts;");
        
        while (query.executeStep()) {
            Prompt prompt;
            prompt.name = query.getColumn(0).getString();
            
            std::string descStr = query.getColumn(1).getString();
            if (!descStr.empty()) {
                prompt.description = descStr;
            }
            
            json argsJson = json::parse(query.getColumn(2).getString());
            for (const auto& argJson : argsJson) {
                prompt.arguments.push_back(PromptArgument::from_json(argJson));
            }
            
            prompts.push_back(std::move(prompt));
        }
        
        return prompts;
    }

    void PromptDao::update(int id, const Prompt& prompt) {
        json argsJson = json::array();
        for (const auto& arg : prompt.arguments) {
            argsJson.push_back(arg.to_json());
        }
        
        SQLite::Statement query(db_, "UPDATE prompts SET name = ?, description = ?, arguments = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;");
        query.bind(1, prompt.name);
        query.bind(2, prompt.description.value_or(""));
        query.bind(3, argsJson.dump());
        query.bind(4, id);
        query.exec();
    }

    void PromptDao::deleteById(int id) {
        SQLite::Statement query(db_, "DELETE FROM prompts WHERE id = ?;");
        query.bind(1, id);
        query.exec();
    }

    bool PromptDao::exists(const std::string& name) {
        SQLite::Statement query(db_, "SELECT 1 FROM prompts WHERE name = ?;");
        query.bind(1, name);
        return query.executeStep();
    }

    PromptMessageDao::PromptMessageDao(SQLite::Database& db) : db_(db) {}

    void PromptMessageDao::create(int promptId, const PromptMessage& message, int orderIndex) {
        std::string roleStr = (message.role == Role::User) ? "user" : "assistant";
        SQLite::Statement query(db_, "INSERT INTO prompt_messages (prompt_id, role, content, order_index) VALUES (?, ?, ?, ?);");
        query.bind(1, promptId);
        query.bind(2, roleStr);
        query.bind(3, message.content.dump());
        query.bind(4, orderIndex);
        query.exec();
    }

    std::vector<PromptMessage> PromptMessageDao::getByPromptId(int promptId) {
        std::vector<PromptMessage> messages;
        SQLite::Statement query(db_, "SELECT role, content FROM prompt_messages WHERE prompt_id = ? ORDER BY order_index;");
        query.bind(1, promptId);
        
        while (query.executeStep()) {
            PromptMessage message;
            std::string roleStr = query.getColumn(0).getString();
            message.role = (roleStr == "user") ? Role::User : Role::Assistant;
            message.content = json::parse(query.getColumn(1).getString());
            messages.push_back(std::move(message));
        }
        
        return messages;
    }

    void PromptMessageDao::deleteByPromptId(int promptId) {
        SQLite::Statement query(db_, "DELETE FROM prompt_messages WHERE prompt_id = ?;");
        query.bind(1, promptId);
        query.exec();
    }
}

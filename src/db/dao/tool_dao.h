#pragma once

#include "../database.h"
#include "../mcp/types.h"
#include <SQLiteCpp/SQLiteCpp.h>

namespace mcp {
    class ToolDao {
    public:
        explicit ToolDao(SQLite::Database& db);
        
        int create(const Tool& tool);
        Tool getById(int id);
        Tool getByName(const std::string& name);
        std::vector<Tool> list();
        void update(int id, const Tool& tool);
        void deleteById(int id);
        bool exists(const std::string& name);
        
    private:
        SQLite::Database& db_;
    };
}

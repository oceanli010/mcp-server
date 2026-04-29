#pragma once

#include "../database.h"
#include "../mcp/types.h"
#include <SQLiteCpp/SQLiteCpp.h>

namespace mcp {
    class ResourceDao {
    public:
        explicit ResourceDao(SQLite::Database& db);
        
        int create(const Resources& resource);
        Resources getById(int id);
        Resources getByUri(const std::string& uri);
        std::vector<Resources> list();
        void update(int id, const Resources& resource);
        void deleteById(int id);
        bool exists(const std::string& uri);
        
    private:
        SQLite::Database& db_;
    };
}

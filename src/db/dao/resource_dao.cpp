#include "resource_dao.h"

namespace mcp {
    ResourceDao::ResourceDao(SQLite::Database& db) : db_(db) {}

    int ResourceDao::create(const Resources& resource) {
        SQLite::Statement query(db_, "INSERT INTO resources (uri, name, description, mime_type) VALUES (?, ?, ?, ?);");
        query.bind(1, resource.uri);
        query.bind(2, resource.name);
        query.bind(3, resource.description.value_or(""));
        query.bind(4, resource.mime_type.value_or(""));
        query.exec();
        return static_cast<int>(db_.getLastInsertRowid());
    }

    Resources ResourceDao::getById(int id) {
        SQLite::Statement query(db_, "SELECT uri, name, description, mime_type FROM resources WHERE id = ?;");
        query.bind(1, id);
        
        if (query.executeStep()) {
            Resources resource;
            resource.uri = query.getColumn(0).getString();
            resource.name = query.getColumn(1).getString();
            
            std::string descStr = query.getColumn(2).getString();
            if (!descStr.empty()) {
                resource.description = descStr;
            }
            
            std::string mimeStr = query.getColumn(3).getString();
            if (!mimeStr.empty()) {
                resource.mime_type = mimeStr;
            }
            
            return resource;
        }
        
        throw std::runtime_error("Resource not found with id: " + std::to_string(id));
    }

    Resources ResourceDao::getByUri(const std::string& uri) {
        SQLite::Statement query(db_, "SELECT id, uri, name, description, mime_type FROM resources WHERE uri = ?;");
        query.bind(1, uri);
        
        if (query.executeStep()) {
            Resources resource;
            resource.uri = query.getColumn(1).getString();
            resource.name = query.getColumn(2).getString();
            
            std::string descStr = query.getColumn(3).getString();
            if (!descStr.empty()) {
                resource.description = descStr;
            }
            
            std::string mimeStr = query.getColumn(4).getString();
            if (!mimeStr.empty()) {
                resource.mime_type = mimeStr;
            }
            
            return resource;
        }
        
        throw std::runtime_error("Resource not found with uri: " + uri);
    }

    std::vector<Resources> ResourceDao::list() {
        std::vector<Resources> resources;
        SQLite::Statement query(db_, "SELECT uri, name, description, mime_type FROM resources;");
        
        while (query.executeStep()) {
            Resources resource;
            resource.uri = query.getColumn(0).getString();
            resource.name = query.getColumn(1).getString();
            
            std::string descStr = query.getColumn(2).getString();
            if (!descStr.empty()) {
                resource.description = descStr;
            }
            
            std::string mimeStr = query.getColumn(3).getString();
            if (!mimeStr.empty()) {
                resource.mime_type = mimeStr;
            }
            
            resources.push_back(std::move(resource));
        }
        
        return resources;
    }

    void ResourceDao::update(int id, const Resources& resource) {
        SQLite::Statement query(db_, "UPDATE resources SET uri = ?, name = ?, description = ?, mime_type = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;");
        query.bind(1, resource.uri);
        query.bind(2, resource.name);
        query.bind(3, resource.description.value_or(""));
        query.bind(4, resource.mime_type.value_or(""));
        query.bind(5, id);
        query.exec();
    }

    void ResourceDao::deleteById(int id) {
        SQLite::Statement query(db_, "DELETE FROM resources WHERE id = ?;");
        query.bind(1, id);
        query.exec();
    }

    bool ResourceDao::exists(const std::string& uri) {
        SQLite::Statement query(db_, "SELECT 1 FROM resources WHERE uri = ?;");
        query.bind(1, uri);
        return query.executeStep();
    }
}

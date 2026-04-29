#include "migration_manager.h"
#include "logger.h"
#include <vector>

namespace mcp {
    MigrationManager::MigrationManager(SQLite::Database& db) : db_(db) {}

    void MigrationManager::runMigrations() {
        MCP_LOG_INFO("Starting database migrations");
        
        initializeMigrationTable();
        int currentVersion = getCurrentVersion();
        
        const std::vector<std::pair<int, std::string>> migrations = {
            {1, R"(
CREATE TABLE IF NOT EXISTS tools (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,
    description TEXT,
    input_schema TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_tools_name ON tools(name);
            )"},
            {2, R"(
CREATE TABLE IF NOT EXISTS resources (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    uri TEXT UNIQUE NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    mime_type TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_resources_uri ON resources(uri);
CREATE INDEX IF NOT EXISTS idx_resources_name ON resources(name);
            )"},
            {3, R"(
CREATE TABLE IF NOT EXISTS prompts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,
    description TEXT,
    arguments TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_prompts_name ON prompts(name);
            )"},
            {4, R"(
CREATE TABLE IF NOT EXISTS prompt_messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    prompt_id INTEGER NOT NULL,
    role TEXT NOT NULL,
    content TEXT NOT NULL,
    order_index INTEGER NOT NULL DEFAULT 0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_prompt_messages_prompt_id ON prompt_messages(prompt_id);
            )"},
            {5, R"(
CREATE TABLE IF NOT EXISTS server_info (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    version TEXT NOT NULL,
    capabilities TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
            )"}
        };
        
        for (const auto& [version, content] : migrations) {
            if (version > currentVersion && !migrationAlreadyApplied(version)) {
                try {
                    executeMigration(version, content);
                    updateMigrationVersion(version);
                    MCP_LOG_INFO("Applied migration version " + std::to_string(version));
                } catch (const std::exception& e) {
                    MCP_LOG_ERROR("Failed to apply migration version " + std::to_string(version) + ": " + std::string(e.what()));
                    throw;
                }
            }
        }
        
        MCP_LOG_INFO("Database migrations complete");
    }

    int MigrationManager::getCurrentVersion() {
        SQLite::Statement query(db_, "SELECT MAX(version) FROM schema_migrations;");
        if (query.executeStep()) {
            return query.getColumn(0).isNull() ? 0 : query.getColumn(0).getInt();
        }
        return 0;
    }

    void MigrationManager::initializeMigrationTable() {
        std::string sql = R"(
            CREATE TABLE IF NOT EXISTS schema_migrations (
                version INTEGER PRIMARY KEY,
                applied_at TEXT DEFAULT (datetime('now'))
            );
            INSERT OR IGNORE INTO schema_migrations (version) VALUES (0);
        )";
        db_.exec(sql);
    }

    void MigrationManager::executeMigration(int version, const std::string& content) {
        SQLite::Transaction transaction(db_);
        db_.exec(content);
        transaction.commit();
    }

    void MigrationManager::updateMigrationVersion(int version) {
        SQLite::Statement query(db_, "INSERT INTO schema_migrations (version) VALUES (?);");
        query.bind(1, version);
        query.exec();
    }

    bool MigrationManager::migrationAlreadyApplied(int version) {
        SQLite::Statement query(db_, "SELECT 1 FROM schema_migrations WHERE version = ?;");
        query.bind(1, version);
        return query.executeStep();
    }
}

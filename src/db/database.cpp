#include "database.h"
#include "config.h"
#include "logger.h"
#include <filesystem>
#include <stdexcept>

namespace mcp {
    Database& Database::getInstance() {
        static Database instance;
        return instance;
    }

    void Database::init() {
        std::lock_guard<std::mutex> lock(dbMutex_);
        if (initialized_) {
            MCP_LOG_WARN("Database already initialized");
            return;
        }

        try {
            dbFilePath_ = MCP_CONFIG.getDbFilePath();
            useWAL_ = MCP_CONFIG.getDbUseWAL();
            busyTimeout_ = MCP_CONFIG.getDbBusyTimeout();

            ensureDirectoryExists();
            
            // 创建一个测试连接
            auto testConn = createConnection();
            
            initialized_ = true;
            MCP_LOG_INFO("Database initialized at: " + dbFilePath_);
        } catch (const std::exception& e) {
            MCP_LOG_ERROR("Failed to initialize database: " + std::string(e.what()));
            throw;
        }
    }

    void Database::shutdown() {
        std::lock_guard<std::mutex> lock(dbMutex_);
        if (!initialized_) {
            return;
        }
        
        initialized_ = false;
        MCP_LOG_INFO("Database shutdown complete");
    }

    bool Database::isInitialized() const {
        return initialized_;
    }

    std::unique_ptr<SQLite::Database> Database::getConnection() {
        std::lock_guard<std::mutex> lock(dbMutex_);
        if (!initialized_) {
            throw std::runtime_error("Database not initialized");
        }
        return createConnection();
    }

    void Database::releaseConnection(std::unique_ptr<SQLite::Database>&& conn) {
        // SQLiteCpp 的连接会在 unique_ptr 析构时自动关闭
        (void)conn; // 防止未使用警告
    }

    std::unique_ptr<SQLite::Database> Database::createConnection() {
        auto db = std::make_unique<SQLite::Database>(
            dbFilePath_,
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        
        db->setBusyTimeout(busyTimeout_);
        
        if (useWAL_) {
            db->exec("PRAGMA journal_mode=WAL;");
            db->exec("PRAGMA synchronous=NORMAL;");
        }
        
        // 启用外键约束
        db->exec("PRAGMA foreign_keys=ON;");
        
        return db;
    }

    void Database::ensureDirectoryExists() {
        std::filesystem::path dbPath(dbFilePath_);
        std::filesystem::create_directories(dbPath.parent_path());
    }
}

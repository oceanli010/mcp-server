#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>

namespace mcp {
    class Database {
    public:
        static Database& getInstance();

        std::unique_ptr<SQLite::Database> getConnection();
        void releaseConnection(std::unique_ptr<SQLite::Database>&& conn);

        void init();
        void shutdown();

        bool isInitialized() const;

    private:
        Database() = default;
        ~Database() = default;

        std::string dbFilePath_;
        std::mutex dbMutex_;
        std::atomic<bool> initialized_{false};
        bool useWAL_{true};
        int busyTimeout_{5000};

        std::unique_ptr<SQLite::Database> createConnection();
        void ensureDirectoryExists();
    };

#define MCP_DB Database::getInstance()
}

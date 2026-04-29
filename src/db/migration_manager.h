#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <string>

namespace mcp {
    class MigrationManager {
    public:
        explicit MigrationManager(SQLite::Database& db);
        void runMigrations();
        int getCurrentVersion();

    private:
        void initializeMigrationTable();
        void executeMigration(int version, const std::string& content);
        void updateMigrationVersion(int version);
        bool migrationAlreadyApplied(int version);

        SQLite::Database& db_;
    };
}

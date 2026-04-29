add_library(mcp_lib STATIC
        config/config.cpp
        logger/logger.cpp
        auth/auth.cpp
        auth/auth.h
        json_rpc/stdio_jsonrpc.cpp
        json_rpc/json_serialization.cpp
        json_rpc/http_jsonrpc.cpp
        mcp/types.cpp
        mcp/types.h
        mcp/mcp_server.cpp
        mcp/mcp_server.h
        db/database.cpp
        db/dao/tool_dao.cpp
        db/dao/resource_dao.cpp
        db/dao/prompt_dao.cpp
        db/repository.cpp
        db/migration_manager.cpp
)

target_include_directories(mcp_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/logger
        ${CMAKE_CURRENT_SOURCE_DIR}/auth
        ${CMAKE_CURRENT_SOURCE_DIR}/json_rpc
        ${CMAKE_CURRENT_SOURCE_DIR}/mcp
        ${CMAKE_CURRENT_SOURCE_DIR}/db
        ${CMAKE_CURRENT_SOURCE_DIR}/db/dao
)

find_package(httplib REQUIRED)
find_package(OpenSSL REQUIRED)

set(SQLITE3_LIBRARY "/home/oceanli/CLionProjects/mcp_server/cmake-build-debug/vcpkg_installed/x64-linux/lib/libsqlite3.a")

target_link_libraries(mcp_lib
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        httplib::httplib
        /usr/lib/x86_64-linux-gnu/libcurl.so
        SQLiteCpp
        ${SQLITE3_LIBRARY}
        OpenSSL::SSL
        OpenSSL::Crypto
)

add_executable(mcp_server
        mcp_server_main.cpp
)

target_link_libraries(mcp_server
        mcp_lib
)

add_library(mcp_client_sdk STATIC
        mcp_client/mcp_client.cpp
)

target_include_directories(mcp_client_sdk PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/mcp
        ${CMAKE_CURRENT_SOURCE_DIR}/mcp_client
)

target_link_libraries(mcp_client_sdk
        mcp_lib
)
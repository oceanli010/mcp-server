add_library(mcp_lib STATIC
        config/config.cpp
        logger/logger.cpp
        json_rpc/stdio_jsonrpc.cpp
        json_rpc/json_serialization.cpp
        json_rpc/http_jsonrpc.cpp
        mcp/types.cpp
        mcp/types.h
        mcp/mcp_server.cpp
        mcp/mcp_server.h
)

target_include_directories(mcp_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/logger
        ${CMAKE_CURRENT_SOURCE_DIR}/json_rpc
        ${CMAKE_CURRENT_SOURCE_DIR}/mcp
)

find_package(httplib REQUIRED)
target_link_libraries(mcp_lib
        Threads::Threads
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        httplib::httplib
)

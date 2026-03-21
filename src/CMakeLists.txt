add_library(mcp_lib STATIC
        config/config.cpp
        logger/logger.cpp
        json_rpc/stdio_jsonrpc.cpp
        json_rpc/json_serialization.cpp
        json_rpc/http_jsonrpc.cpp
)

target_include_directories(mcp_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/config
        ${CMAKE_CURRENT_SOURCE_DIR}/logger
        ${CMAKE_CURRENT_SOURCE_DIR}/json_rpc
        ${CMAKE_CURRENT_SOURCE_DIR}/mcp
)

target_link_libraries(mcp_lib
        Threads::Threads
)

target_link_libraries(mcp_lib nlohmann_json::nlohmann_json)

target_link_libraries(mcp_lib spdlog::spdlog)

find_package(httplib REQUIRED)
target_link_libraries(mcp_lib httplib::httplib)

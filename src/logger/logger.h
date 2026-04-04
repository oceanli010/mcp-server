#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/fmt/ostr.h>

#include <memory>
#include <string>

namespace mcp {
    namespace logger {
        class Logger {
        public:
            //单例模式构造
            static Logger& getInstance();

            //初始化
            void init(const std::string& logger_name = "mcp",
                      const std::string& log_file_path = "",
                      size_t max_file_size = 1024 * 1024 * 1024,
                      size_t max_file_count = 5,
                      bool console_output = true);

            //设置日志等级
            void setLevel(spdlog::level::level_enum level);

            //获取日志实例
            std::shared_ptr<spdlog::logger> getLogger();

            void flush();
            void shutdown();

            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

        private:
            Logger() = default;
            ~Logger();

            std::shared_ptr<spdlog::logger> logger_;
            bool initialized_ = false;
        };
    }
}

#define MCP_LOG_INIT(name, ...) \
    mcp::logger::Logger::getInstance().init(name, __VA_ARGS__)

#define MCP_LOG_SET_LEVEL(level, ...) \
    mcp::logger::Logger::getInstance().setLevel(level)

#define MCP_LOG_FLUSH() \
    mcp::logger::Logger::getInstance().flush()

#define MCP_LOG_SHUTDOWN() \
    mcp::logger::Logger::getInstance().shutdown()

#define MCP_LOG_INFO(fmt, ...) \
    do { \
        auto logger = mcp::logger::Logger::getInstance().getLogger(); \
        if (logger) logger->info("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define MCP_LOG_WARN(fmt, ...) \
    do { \
        auto logger = mcp::logger::Logger::getInstance().getLogger(); \
        if (logger) logger->warn("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define MCP_LOG_ERROR(fmt, ...) \
    do{ \
        auto logger = mcp::logger::Logger::getInstance().getLogger(); \
        if (logger) logger->error("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define MCP_LOG_CRITICAL(fmt, ...) \
    do { \
        auto logger = mcp::logger::Logger::getInstance().getLogger(); \
        if (logger) logger->critical("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define MCP_LOG_DEBUG(fmt, ...) \
    do { \
        auto logger = mcp::logger::Logger::getInstance().getLogger(); \
        if (logger) logger->debug("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define MCP_LOG_TRACE(fmt, ...) \
    do { \
        auto logger = mcp::logger::Logger::getInstance().getLogger(); \
        if (logger) logger->trace("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while(0)

#define MCP_LOG_DEBUG_IF(condition, ...) \
    do { \
        if (condition) { \
            auto logger = mcp::logger::Logger::getInstance().getLogger(); \
            if (logger) logger->debug(__VA_ARGS__); \
        } \
    } while(0)

#define MCP_LOG_ERROR_IF(condition, ...) \
    do { \
        if (condition) { \
            auto logger = mcp::logger::Logger::getInstance().getLogger(); \
            if (logger) logger->error(__VA_ARGS__); \
        } \
    } while(0)

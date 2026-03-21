#include "logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <filesystem>
#include <iostream>

namespace mcp {

    namespace logger {
        Logger& Logger::getInstance() {
            static Logger instance;
            return instance;
        }

        void Logger::init(const std::string logger_name,
                      const std::string log_file_path,
                      size_t max_file_size,
                      size_t max_file_count,
                      bool console_output) {
            if (initialized_) {
                MCP_LOG_WARN("Logger already initialized");
                return;
            }

            try {
                std::vector<spdlog::sink_ptr> sinks;

                //创建控制台sink
                if (console_output) {
                    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                    console_sink->set_pattern("[%H:%M:%S.%e] [%^%l%$] [%n] %v");
                    sinks.push_back(console_sink);
                }

                //创建文件sink
                if (!log_file_path.empty()) {
                    std::filesystem::path loh_path(log_file_path);
                    std::filesystem::path log_dir = loh_path.parent_path();
                    std::filesystem::create_directory(log_dir);
                    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file_path, max_file_size, max_file_count);
                    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [%t] %v]");
                    sinks.push_back(file_sink);
                }

                //创建logger实例并注册到全局
                logger_ = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
                logger_->set_level(spdlog::level::info);
                logger_->flush_on(spdlog::level::info);
                spdlog::register_logger(logger_);
                initialized_ = true;
                logger_->info("Logger initialized - name: {}, path: {}", logger_name, log_file_path);

            } catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Logger initilazed fail: " << ex.what() << std::endl;
                throw;
            }
        }

        void Logger::setLevel(spdlog::level::level_enum level) {
            if (logger_) {
                logger_->set_level(level);
                logger_->info("Logger set level: {}", level);
            }
        }

        std::shared_ptr<spdlog::logger> Logger::getLogger() {
            if (!initialized_) {
                init();
            }
            return logger_;
        }

        void Logger::flush() {
            if (logger_) {
                logger_->flush();
            }
        }

        void Logger::shutdown() {
            if (!initialized_) {
                return;
            }
            try {
                if (logger_) {
                    logger_->flush();
                }
            } catch (const spdlog::spdlog_ex& ex) {
                std::cerr << "Logger shutdown fail: " << ex.what() << std::endl;
            }
            spdlog::shutdown();
            logger_.reset();
            initialized_ = false;
        }

        Logger::~Logger() {
            shutdown();
        }
    }
}
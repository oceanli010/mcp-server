#include "config.h"
#include "logger.h"

#include <fstream>
#include <iostream>

#define MIN_PORT 1
#define MAX_PORT 65535

namespace mcp {
    //单例模式，创建全局唯一实例
    Config& Config::getInstance() {
        static Config instance;
        return instance;
    }

    bool Config::loadConfigFile(const std::string& config_file_path) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        config_file_path_ = config_file_path;

        try {
            //创建输入文件流，读取配置文件信息
            std::ifstream config_file(config_file_path_);
            if (!config_file.is_open()) {
                MCP_LOG_ERROR("Failed to open config file: " + config_file_path_);
                return false;
            }

            //将配置信息写入config_data_
            config_file >> config_data_;
            config_file.close();

            //设置默认值
            setDefaults();

            //检测配置是否合法
            if (!validateConfig()) {
                MCP_LOG_ERROR("Failed to validate config file: " + config_file_path_);
                return false;
            }

            is_loaded_ = true;
            return true;
        } catch (const std::exception& e) {
            MCP_LOG_ERROR("Failed to load config file: " + std::string(e.what()));
            return false;
        }
    }

    bool Config::validateConfig() const {
        //检查server字段是否存在
        if (!config_data_.contains("server")) {
            MCP_LOG_ERROR("Invalid server configuration");
            return false;
        }

        //检查端口号合法性
        int port = config_data_["server"].value("port", 8080);
        if (port < MIN_PORT || port > MAX_PORT) {
            MCP_LOG_ERROR("Invalid port number: ", port);
            return false;
        }

        //检查logging字段合法性
        if (config_data_.contains("logging")) {
            std::string log_level = config_data_["logging"].value("log_level", std::string("info"));
            if (log_level != "info" && log_level != "debug" && log_level != "trace"&&
                log_level != "warn" && log_level != "error" && log_level != "critical") {
                    MCP_LOG_ERROR("Invalid log level: " + log_level);
                    return false;
                }

            size_t log_file_size = config_data_["logging"].value("log_file_size", 10 * 1024 * 1024);
            if (log_file_size <= 0) {
                MCP_LOG_ERROR("Invalid log file size: " + std::to_string(log_file_size));
                return false;
            }
        }

        return true;
    }

    //设置默认值
    void Config::setDefaults() {
        if (config_data_.contains("server")) {
            auto& server = config_data_["server"];
            if (!server.contains("port")) {
                server["port"] = 8080;  //默认端口号
            }
        }

        if (!config_data_.contains("logging")) {
            config_data_["logging"] = json::object();
        }

        auto& logging = config_data_["logging"];
        if (!logging.contains("log_level")) {
            logging["log_level"] = "info";
        }
        if (!logging.contains("log_file_size")) {
            logging["log_file_size"] = 10 * 1024 * 1024;
        }
        if (!logging.contains("log_file_path")) {
            logging["log_file_path"] = "logs/server.log";
        }
        if (!logging.contains("log_file_count")) {
            logging["log_file_count"] = 5;
        }
        if (!logging.contains("log_console_output")) {
            logging["log_console_output"] = true;
        }
        
        if (!config_data_.contains("auth")) {
            config_data_["auth"] = json::object();
        }
        
        auto& auth = config_data_["auth"];
        if (!auth.contains("api_keys")) {
            auth["api_keys"] = json::array();
        }
    }

    int Config::getServerPort() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return config_data_["server"].value("port", 8080);
    }

    std::string Config::getLogPath() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return config_data_["logging"].value("log_file_path", "");
    }

    std::string Config::getLogLevel() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return config_data_["logging"].value("log_level", std::string("info"));
    }

    size_t Config::getLogFileSize() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return config_data_["logging"].value("log_file_size", 10 * 1024 * 1024);
    }

    int Config::getLogFileCount() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return config_data_["logging"].value("log_file_count", 5);
    }

    bool Config::getLogConsoleOutput() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return config_data_["logging"].value("log_console_output", true);
    }
    
    std::vector<std::string> Config::getApiKeys() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<std::string> api_keys;
        if (config_data_.contains("auth") && config_data_["auth"].contains("api_keys")) {
            for (const auto& key : config_data_["auth"]["api_keys"]) {
                api_keys.push_back(key.get<std::string>());
            }
        }
        return api_keys;
    }
}

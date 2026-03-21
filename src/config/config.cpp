#include "config.h"

#include <fstream>
#include <iostream>

#define MIN_PORT 1
#define MAX_PORT 65535

namespace mcp {
    Config& Config::getInstance() {
        static Config instance;
        return instance;
    }

    bool Config::loadConfigFile(const std::string& config_file_path) {
        config_file_path_ = config_file_path;

        try {
            std::ifstream config_file(config_file_path_);
            if (!config_file.is_open()) {
                std::cerr << "Error opening file: " << config_file_path_ << std::endl;
                return false;
            }

            config_file >> config_data_;
            config_file.close();

            setDefaults();

            if (!validateConfig()) {
                std::cerr << "Config is Invalid: " << config_file_path_ << std::endl;
                return false;
            }

            is_loaded_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Config file error: " << e.what() << std::endl;
            return false;
        }
    }

    bool Config::validateConfig() const {
            if (!config_data_.contains("server")) {
                std::cerr << "Config file does not contain \"server\"." << std::endl;
                return false;
            }

            int port = config_data_["server"].value("port", 8080);
            if (port < MIN_PORT || port > MAX_PORT) {
                std::cerr << "Invalid port number: " << port << std::endl;
                return false;
            }

            if (config_data_.contains("logging")) {
                std::string log_level = config_data_["logging"].value("log_level", std::string("info"));
                if (log_level != "info" && log_level != "debug" && log_level != "trace"&&
                    log_level != "warn" && log_level != "error" && log_level != "critical") {
                    std::cerr << "Invalid logging level: " << log_level << std::endl;
                    return false;
                }

                size_t log_file_size = config_data_["logging"].value("log_file_size", 10 * 1024 * 1024);
                if (log_file_size <= 0) {
                    std::cerr << "Invalid log file size: " << log_file_size << std::endl;
                    return false;
                }
            }

            return true;
        }

        void Config::setDefaults() {
            if (!config_data_.contains("server")) {
                config_data_["server"] = json::object();
            }

            auto& server = config_data_["server"];
            if (!server.contains("port")) {
                server["port"] = 8080;
            }

            if (!server.contains("logging")) {
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
        }
}

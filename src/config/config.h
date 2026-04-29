///配置模块
///在../config/server.json中读取配置

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace mcp {
    using json = nlohmann::json;

    class Config {
    public:
        //单例模式构造配置
        static Config& getInstance();

        //加载配置文件
        bool loadConfigFile(const std::string& config_file_path_);

        //返回配置加载状态
        bool isLoaded() const {return is_loaded_;};

        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        //get方法，用于获取配置信息
        int getServerPort() const;

        std::string getLogPath() const;

        std::string getLogLevel() const;

        size_t getLogFileSize() const;

        int getLogFileCount() const;

        bool getLogConsoleOutput() const;
    
        std::vector<std::string> getApiKeys() const;

        // 数据库配置
        std::string getDbType() const;
        std::string getDbFilePath() const;
        bool getDbUseWAL() const;
        int getDbBusyTimeout() const;

    private:
        Config() = default;
        ~Config() = default;

        //检查配置合法性
        bool validateConfig() const;

        //设置默认值
        void setDefaults();

        bool is_loaded_ = false;
        json config_data_;
        std::string config_file_path_;
        mutable std::shared_mutex mutex_;
    };

    #define MCP_CONFIG Config::getInstance()
}

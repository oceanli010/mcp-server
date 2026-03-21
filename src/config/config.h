#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <memory>

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
    };

    #define MCP_CONFIG Config::getInstance()
}
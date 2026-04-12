///API认证模块
///对请求进行认证检查

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>

namespace mcp {
    class AuthManager {
    public:
        static AuthManager& getInstance();

        void init(const std::vector<std::string>& api_keys);
        
        bool validateApiKey(const std::string& api_key) const; //验证API Key
        bool isEnabled() const;
        
        void addApiKey(const std::string& api_key);     //添加API Key
        void removeApiKey(const std::string& api_key);  //移除API Key
        std::vector<std::string> listApiKeys() const;
        
        AuthManager(const AuthManager&) = delete;
        AuthManager& operator=(const AuthManager&) = delete;
        
    private:
        AuthManager() = default;
        ~AuthManager() = default;
        
        std::vector<std::string> api_keys_;
        bool enabled_ = false;
        mutable std::mutex mutex_;
    };
    
    #define MCP_AUTH AuthManager::getInstance()
}

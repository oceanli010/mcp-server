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
        
        bool validateApiKey(const std::string& api_key) const;
        bool isEnabled() const;
        
        void addApiKey(const std::string& api_key);
        void removeApiKey(const std::string& api_key);
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

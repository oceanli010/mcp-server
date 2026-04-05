#include "auth.h"
#include "logger.h"

#include <algorithm>

namespace mcp {
    AuthManager& AuthManager::getInstance() {
        static AuthManager instance;
        return instance;
    }
    
    void AuthManager::init(const std::vector<std::string>& api_keys) {
        std::lock_guard<std::mutex> lock(mutex_);
        api_keys_ = api_keys;
        enabled_ = !api_keys_.empty();
        MCP_LOG_INFO("AuthManager initialized with {} API keys, enabled: {}", api_keys_.size(), enabled_);
    }
    
    bool AuthManager::validateApiKey(const std::string& api_key) const {
        if (!enabled_) {
            return true;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        return std::find(api_keys_.begin(), api_keys_.end(), api_key) != api_keys_.end();
    }
    
    bool AuthManager::isEnabled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return enabled_;
    }
    
    void AuthManager::addApiKey(const std::string& api_key) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (std::find(api_keys_.begin(), api_keys_.end(), api_key) == api_keys_.end()) {
            api_keys_.push_back(api_key);
            enabled_ = true;
            MCP_LOG_INFO("Added API key");
        }
    }
    
    void AuthManager::removeApiKey(const std::string& api_key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find(api_keys_.begin(), api_keys_.end(), api_key);
        if (it != api_keys_.end()) {
            api_keys_.erase(it);
            enabled_ = !api_keys_.empty();
            MCP_LOG_INFO("Removed API key");
        }
    }
    
    std::vector<std::string> AuthManager::listApiKeys() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return api_keys_;
    }
}

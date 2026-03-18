#include "jsonrpc.h"
#include "json_serialization.h"
#include "logger.h"

#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace mcp {
    void JsonRpcDispatcher::registerHandler(const std::string& method, Handler handler) {
        handlers_[method] = std::move(handler);
    }

    bool JsonRpcDispatcher::hasHandler(const std::string& method) {
        return handlers_.find(method) != handlers_.end();
    }

    json JsonRpcDispatcher::call(const std::string& method, const json& params) const {
        auto it = handlers_.find(method);
        if (it == handlers_.end()) {
            throw std::runtime_error("Method " + method + " not found");
        }
        return it->second(params);
    }

    bool StdioJsonRpcServer::readMessage(std::string& out_body) {
        out_body.clear();

        std::string line;
        size_t content_length = 0;
        bool found_content_length = false;

        while (std::getline(in_, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) {
                break;
            }

            auto colon = line.find(':');
            if (colon == std::string::npos) {
                continue;
            }

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            size_t pos = value.find_first_not_of(' ');
            if (pos == std::string::npos) {
                MCP_LOG_DEBUG("Value: {}", value);
                value = value.substr(pos);
            }

            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
            if (key == "content-length") {
                try {
                    content_length = static_cast<size_t>(std::stoul(value));
                    found_content_length = true;
                } catch (...) {
                    MCP_LOG_ERROR("Invalid content length: {}", value);
                    return false;
                }
            } else if (key == "content-type") {
                MCP_LOG_DEBUG("Content-type: {}", value);
            } else {
                MCP_LOG_DEBUG("Ignore Header: {}:{}", key, value);
            }
        }
        if (!found_content_length) {
            MCP_LOG_ERROR("content_length not found");
            return false;
        }

        if (content_length == 0) {
            MCP_LOG_DEBUG("Empty content");
            return true;
        }

        out_body.resize(content_length);
        size_t total_read = 0;
        while (total_read < content_length) {
            const std::streamsize to_read = static_cast<std::streamsize>(content_length - total_read);
            in_.read(&out_body[total_read], to_read);

            const std::streamsize just_read = in_.gcount();
            if (just_read <= 0) break;
            total_read += static_cast<size_t>(just_read);

            if (!in_.good() && !in_.eof()) {
                break;
            }
        }
        if (total_read != content_length) {
            MCP_LOG_ERROR("Incomplete message: expected {} bytes, got {}", content_length, total_read);
            return false;
        }
        return true;
    }
}

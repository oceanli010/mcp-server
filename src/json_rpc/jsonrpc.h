#pragma once

#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <iostream>

namespace mcp {
    using json = nlohmann::json;

    struct JsonRpcError {
        int code;
        std::string message;
        std::optional<json> data;
    };

    struct JsonRpcRequest {
        std::string json_rpc = "2.0";
        std::optional<json> id;
        std::string method;
        std::optional<json> params;
    };

    struct JsonRpcResponse {
        std::string json_rpc = "2.0";
        std::string id;
        std::optional<json> result;
        std::optional<JsonRpcError> error;
    };

    class JsonRpcDispatcher {
    public:
        using Handler = std::function<json(const json& params)>;

        void registerHandler(const std::string& method, Handler handler);

        bool hasHandler(const std::string& method);

        json call(const std::string& method, const json& params) const;

    private:
        std::unordered_map<std::string, Handler> handlers_;
    };

    class StdioJsonRpcServer {
    public:
        explicit StdioJsonRpcServer(JsonRpcDispatcher& dispatcher);

        StdioJsonRpcServer(JsonRpcDispatcher& dispatcher, std::istream& in, std::ostream& out);

        void run();

    private:
        JsonRpcDispatcher& dispatcher_;
        std::istream& in_;
        std::ostream& out_;

        bool readMessage(std::string& out_body);

        void writeMessage(const json& msg);

        JsonRpcResponse handleRequest(const JsonRpcRequest& req);
    };

}
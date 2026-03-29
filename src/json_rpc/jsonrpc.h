#pragma once

#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <iostream>
#include <mutex>

namespace mcp {
    using json = nlohmann::json;

    struct JsonRpcError {
        int code;       //错误码
        std::string message;        //错误信息
        std::optional<json> data;   //错误数据
    };

    struct JsonRpcRequest {
        std::string json_rpc = "2.0";
        std::optional<json> id;     //留空为通知
        std::string method;         //请求的方法名
        std::optional<json> params; //请求参数（对象或数组）
    };

    struct JsonRpcResponse {
        std::string json_rpc = "2.0";
        std::optional<json> id;
        std::optional<json> result;         //执行成功的结果
        std::optional<JsonRpcError> error;  //执行失败的错误信息
    };

    class JsonRpcDispatcher {
    public:
        //方法处理器函数类型模板
        using Handler = std::function<json(const json& params)>;

        JsonRpcDispatcher() = default;

        JsonRpcDispatcher(JsonRpcDispatcher&& other) noexcept {
            handlers_ = std::move(other.handlers_);
        }

        JsonRpcDispatcher& operator=(JsonRpcDispatcher&& other) noexcept {
            if (this != &other) {
                handlers_ = std::move(other.handlers_);
            }
            return *this;
        }

        //注册方法处理器
        void registerHandler(const std::string& method, Handler handler);

        //检查注册状态
        bool hasHandler(const std::string& method) const;

        //调用方法处理器
        json call(const std::string& method, const json& params) const;

    private:
        std::unordered_map<std::string, Handler> handlers_;
        mutable std::mutex mutex_;
    };

    class StdioJsonRpcServer {
    public:
        explicit StdioJsonRpcServer(JsonRpcDispatcher&& dispatcher);

        StdioJsonRpcServer(JsonRpcDispatcher&& dispatcher, std::istream& in, std::ostream& out);

        void run();

    private:
        JsonRpcDispatcher dispatcher_;
        std::istream& in_ = std::cin;
        std::ostream& out_ = std::cout;

        bool readMessage(std::string& out_body);

        void writeMessage(const json& msg);

        JsonRpcResponse handleRequest(const JsonRpcRequest& req);
    };

    namespace jsonrpc_errc {
        constexpr int ParseError = -32700;
        constexpr int InvalidRequest = -32600;
        constexpr int MethodNotFound = -32601;
        constexpr int InvalidParams = -32602;
        constexpr int InternalError = -32603;
    }

}
///HTTP模块定义

#pragma once

#include "jsonrpc.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>

namespace mcp {
    class HttpJsonRpcServer {
    public:
        explicit HttpJsonRpcServer(JsonRpcDispatcher&& dispatcher);

        HttpJsonRpcServer(JsonRpcDispatcher&& dispatcher, const std::string& host, int port);

        virtual ~HttpJsonRpcServer();

        HttpJsonRpcServer(const HttpJsonRpcServer&) = delete;
        HttpJsonRpcServer& operator=(const HttpJsonRpcServer&) = delete;

        void run();

        void stop();

        using sseCallback = std::function<void(const std::function<void(const std::string&)>&)>;

        void registerSseEndPoint(const std::string& path, sseCallback callback);    //注册SSE端点

    private:
        // 使用pimpl模式隐藏httplib实现细节
        class Impl;
        std::unique_ptr<Impl> impl_;

        JsonRpcDispatcher dispatcher_;
        std::string host_;
        int port_;
        std::atomic<bool> running_ {false};

        std::string handleRequest(const std::string& request);              //批量请求处理
        JsonRpcResponse processSingleRequest(const JsonRpcRequest& req);    //单个请求处理
        static std::string createErrorResponse(int code, const std::string& message);  //包装错误信息
    };
}
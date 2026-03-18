#pragma once

#include "jsonrpc.h"

namespace mcp {
    void to_json(json& j, const JsonRpcRequest& r);
    void from_json(const json& j, JsonRpcRequest& r);

    void to_json(json& j, const JsonRpcResponse& r);
    void from_json(const json& j, JsonRpcResponse& r);

    void to_json(json& j, const JsonRpcError& r);
    void from_json(const json& j, JsonRpcError& r);
}
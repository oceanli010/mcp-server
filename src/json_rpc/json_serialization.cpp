#include "json_serialization.h"

namespace mcp {
    void to_json(json& j, const JsonRpcRequest& r) {
        j = json {
            {"jsonrpc", "2.0"},
            {"method", r.method}
        };
        if (r.id.has_value()) {
            j.emplace("id", *r.id);
        }
        if (r.params.has_value()) {
            j.emplace("params", *r.params);
        }
    }

    void from_json(const json& j, JsonRpcResponse& r) {
        r.jsonrpc = j.value("jsonrpc", "2.0");
        r.method = j.at("method").get<std::string>();

        auto it_id = j.find("id");
        if (it_id != j.end()) {
            r.id = *it_id;
        }
        auto it_params = j.find("params");
        if (it_params != j.end()) {
            r.params = *it_params;
        }
    }

    void to_json(json& j,const JsonRpcError& r) {
        j = json {
            {"code", r.code},
            {"message", r.message}
        };
        if (r.data.has_value()) {
            j.emplace("data", *r.data);
        }
    }

    void from_json(const json& j, JsonRpcError& r) {
        r.code = j.at("code").get<int>();
        r.message = j.at("message").get<std::string>();
        auto it_data = j.find("data");
        if (it_data != j.end()) {
            r.data = *it_data;
        } else {
            r.dara.reset();
        }
    }

    void to_json(json& j,const JsonRpcRequest& r) {
        j = json {
            {"jsonrpc", "2.0"},
            {"id", r.id},
        };
        if (r.result.has_value()) {
            j.emplace("result", *r.result);
        } else if (r.error.has_value()) {
            j.emplace("error", *r.error);
        }
    }

    void from_json(const json& j, JsonRpcRequest& r) {
        r.jsonrpc = j.value("jsonrpc", "2.0");
        r.id = j.at("id").get<std::string>();

        r.error.reset();
        r.result.reset();

        auto it_error = j.find("error");
        auto it_result = j.find("result");

        if (it_result != j.end()) {
            r.result = *it_result;
        } else if (r.error.has_value()) {
            r.error = it_error->get<JsonRpcError>();
        }
    }
}
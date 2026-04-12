///MCP一些基本结构体定义
///包括Tool(工具）、Resource(资源)和Prompt(提示词)

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace mcp {
    using json = nlohmann::json;

    enum class Role {
        User,
        Assistant
    };

    //工具输入Schema
    struct ToolInputSchema {
        std::string type = "object";    //类型
        json properties;                //属性定义
        std::vector<std::string> requirements;  //必填字段列表

        json to_json() const;
        static ToolInputSchema from_json(const json& j);
    };

    //工具定义
    struct Tool {
        std::string name;       //工具名
        std::string description;    //工具描述
        ToolInputSchema input_schema;   //输入schema

        json to_json() const;
        static Tool from_json(const json& j);
    };

    //工具调用结果
    struct ContentItem {
        std::string type;   //结果类型。可以是下面四种的其中之一：
        std::optional<std::string> text;    //文本
        std::optional<std::string> base64;  //图片
        std::optional<std::string> mime_type;   //mime消息
        std::optional<std::string> uri;     //uri

        json to_json() const;
        static ContentItem from_json(const json& j);
    };

    //工具调用结果
    struct ToolResult {
        std::vector<ContentItem> content_items;
        bool is_error = false;

        json to_json() const;
        static ToolResult from_json(const json& j);
    };

    //资源定义
    struct Resources {
        std::string uri;        //资源uri
        std::string name;       //资源名
        std::optional<std::string> description;     //资源描述
        std::optional<std::string> mime_type;       //mime类型

        json to_json() const;
        static Resources from_json(const json& j);
    };

    //资源内容
    struct ResourcesContent {
        std::string uri;
        std::optional<std::string> mime_type;
        std::string text;   //文本内容
        std::optional<std::string> base64;  //base64编码

        json to_json() const;
        static ResourcesContent from_json(const json& j);
    };

    //提示词参数
    struct PromptArgument {
        std::string name;
        std::optional<std::string> description;
        bool required = false;

        json to_json() const;
        static PromptArgument from_json(const json& j);
    };

    //提示词定义
    struct Prompt {
        std::string name;
        std::optional<std::string> description;
        std::vector<PromptArgument> arguments;

        json to_json() const;
        static Prompt from_json(const json& j);
    };

    //提示此消息
    struct PromptMessage {
        Role role;  //消息角色(user 或 assistant)
        json content;

        json to_json() const;
        static PromptMessage from_json(const json& j);
    };

    //服务器能力
    struct ServerCapabilities {
        //工具能力
        struct ToolCapabilities {
            bool list_changed = false;  //工具列表变更标志

            json to_json() const;
            static ToolCapabilities from_json(const json& j);
        };

        struct ResourcesCapabilities {
            bool subscribed = false;    //资源订阅标志
            bool list_changed = false;  //资源列表变更标志

            json to_json() const;
            static ResourcesCapabilities from_json(const json& j);
        };

        struct PromptCapabilities {
            bool list_changed = false;  //提示词列表变更标志

            json to_json() const;
            static PromptCapabilities from_json(const json& j);
        };

        //三大模块能力和日志配置
        std::optional<ToolCapabilities> tool_capabilities;
        std::optional<ResourcesCapabilities> resources_capabilities;
        std::optional<PromptCapabilities> prompt_capabilities;
        std::optional<json> logging;

        json to_json() const;
        static ServerCapabilities from_json(const json& j);
    };

    //服务器信息定义
    struct ServerInfo {
        std::string name;
        std::string version;

        json to_json() const;
        static ServerInfo from_json(const json& j);
    };

    //MCP Server初始化结果
    struct InitializeResult {
        std::string version;
        ServerCapabilities capabilities;
        ServerInfo info;

        json to_json() const;
        static InitializeResult from_json(const json& j);
    };
}
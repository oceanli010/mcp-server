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

    struct ToolInputSchema {
        std::string type = "object";
        json properties;
        std::vector<std::string> requirements;

        json to_json() const;
        static ToolInputSchema from_json(const json& j);
    };

    struct Tool {
        std::string name;
        std::string description;
        ToolInputSchema input_schema;

        json to_json() const;
        static Tool from_json(const json& j);
    };

    struct ContentItem {
        std::string type;
        std::optional<std::string> text;
        std::optional<std::string> base64;
        std::optional<std::string> mime_type;
        std::optional<std::string> uri;

        json to_json() const;
        static ContentItem from_json(const json& j);
    };

    struct ToolResult {
        std::vector<ContentItem> content_items;
        bool is_error = false;

        json to_json() const;
        static ToolResult from_json(const json& j);
    };

    struct Resources {
        std::string uri;
        std::string name;
        std::optional<std::string> description;
        std::optional<std::string> mime_type;

        json to_json() const;
        static Resources from_json(const json& j);
    };

    struct ResourcesContent {
        std::string uri;
        std::optional<std::string> mime_type;
        std::string text;
        std::optional<std::string> base64;

        json to_json() const;
        static ResourcesContent from_json(const json& j);
    };

    struct PromptArgument {
        std::string name;
        std::optional<std::string> description;
        bool required = false;

        json to_json() const;
        static PromptArgument from_json(const json& j);
    };

    struct Prompt {
        std::string name;
        std::optional<std::string> description;
        std::vector<PromptArgument> arguments;

        json to_json() const;
        static Prompt from_json(const json& j);
    };

    struct PromptMessage {
        Role role;
        json content;

        json to_json() const;
        static PromptMessage from_json(const json& j);
    };

    struct ServerCapabilities {
        struct ToolCapabilities {
            bool list_changed = false;

            json to_json() const;
            static ToolCapabilities from_json(const json& j);
        };

        struct ResourcesCapabilities {
            bool subscribed = false;
            bool list_changed = false;

            json to_json() const;
            static ResourcesCapabilities from_json(const json& j);
        };

        struct PromptCapabilities {
            bool list_changed = false;

            json to_json() const;
            static PromptCapabilities from_json(const json& j);
        };

        std::optional<ToolCapabilities> tool_capabilities;
        std::optional<ResourcesCapabilities> resources_capabilities;
        std::optional<PromptCapabilities> prompt_capabilities;
        std::optional<json> logging;

        json to_json() const;
        static ServerCapabilities from_json(const json& j);
    };

    struct ServerInfo {
        std::string name;
        std::string version;

        json to_json() const;
        static ServerInfo from_json(const json& j);
    };

    struct InitializeResult {
        std::string version;
        ServerCapabilities capabilities;
        ServerInfo info;

        json to_json() const;
        static InitializeResult from_json(const json& j);
    };
}
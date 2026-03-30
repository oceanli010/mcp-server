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
        std::optional<std::string> mine_type;
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
        std::optional<std::string> mine_type;

        json to_json() const;
        static Resources from_json(const json& j);
    };

    struct ResourcesContent {
        std::string uri;
        std::optional<std::string> mine_type;
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
}
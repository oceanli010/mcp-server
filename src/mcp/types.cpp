#include "types.h"

namespace mcp {

    // ToolInputSchema implementation
    json ToolInputSchema::to_json() const {
        json j;
        j["type"] = type;
        j["properties"] = properties;
        if (!requirements.empty()) {
            j["requirements"] = requirements;
        }
        return j;
    }

    ToolInputSchema ToolInputSchema::from_json(const json& j) {
        ToolInputSchema schema;
        schema.type = j.value("type", "object");
        schema.properties = j.value("properties", json::object());
        if (j.contains("requirements")) {
            for (const auto& requirement : j["requirements"]) {
                schema.requirements.push_back(requirement.get<std::string>());
            }
        }
        return schema;
    }

    // Tool implementation
    json Tool::to_json() const {
        json j;
        j["name"] = name;
        j["description"] = description;
        j["input_schema"] = input_schema.to_json();
        return j;
    }

    Tool Tool::from_json(const json& j) {
        Tool tool;
        tool.name = j["name"];
        tool.description = j["description"];
        if (j.contains("input_schema")) {
            tool.input_schema = ToolInputSchema::from_json(j["input_schema"]);
        }
        return tool;
    }

    // ContentItem implementation
    json ContentItem::to_json() const {
        json j;
        j["type"] = type;
        if (text) j["text"] = *text;
        if (base64) j["base64"] = *base64;
        if (mine_type) j["mine_type"] = *mine_type;
        if (uri) j["uri"] = *uri;
        return j;
    }

    ContentItem ContentItem::from_json(const json& j) {
        ContentItem item;
        item.type = j["type"];
        if (j.contains("text")) item.text = j["text"];
        if (j.contains("base64")) item.base64 = j["base64"];
        if (j.contains("mine_type")) item.mine_type = j["mine_type"];
        if (j.contains("uri")) item.uri = j["uri"];
        return item;
    }

    // ToolResult implementation
    json ToolResult::to_json() const {
        json j;
        j["type"] = is_error ? "error" : "result";
        j["content_items"] = json::array();
        for (const auto& item : content_items) {
            j["content_items"].push_back(item.to_json());
        }
        return j;
    }

    ToolResult ToolResult::from_json(const json& j) {
        ToolResult result;
        if (j.contains("content_items")) {
            for (const auto& item : j["content_items"]) {
                result.content_items.push_back(ContentItem::from_json(item));
            }
        }
        result.is_error = j.value("is_error", false);
        return result;
    }

    // Resources implementation
    json Resources::to_json() const {
        json j;
        j["uri"] = uri;
        j["name"] = name;
        if (description) j["description"] = *description;
        if (mine_type) j["mine_type"] = *mine_type;
        return j;
    }

    Resources Resources::from_json(const json& j) {
        Resources resources;
        resources.uri = j["uri"];
        resources.name = j["name"];
        if (j.contains("description")) resources.description = j["description"];
        if (j.contains("mine_type")) resources.mine_type = j["mine_type"];
        return resources;
    }

    // ResourcesContent implementation
    json ResourcesContent::to_json() const {
        json j = {
            {"uri", uri},
            {"text", text}
        };

        if (mine_type.has_value()) {
            j["mineT_type"] = *mine_type;
        }
        if (base64.has_value()) {
            j["base64"] = *base64;
        }
        return j;
    }

    ResourcesContent ResourcesContent::from_json(const json& j) {
        ResourcesContent content;
        content.uri = j["uri"];
        if (j.contains("mine_type")) content.mine_type = j["mine_type"];
        content.text = j["text"];
        if (j.contains("base64")) content.base64 = j["base64"];
        return content;
    }

    // PromptArgument implementation
    json PromptArgument::to_json() const {
        json j;
        j["name"] = name;
        if (description) j["description"] = *description;
        j["required"] = required;
        return j;
    }

    PromptArgument PromptArgument::from_json(const json& j) {
        PromptArgument arg;
        arg.name = j["name"];
        arg.required = j.value("required", false);
        if (j.contains("description")) {
            arg.description = j["description"];
        }
        return arg;
    }

    // Prompt implementation
    json Prompt::to_json() const {
        json j;
        j["name"] = name;
        if (description) j["description"] = *description;
        json arg_json = json::array();
        if (!arguments.empty()) {
            for (const auto& arg : arguments) {
                arg_json.push_back(arg.to_json());
            }
        }
        j["arguments"] = arg_json;
        return j;
    }

    Prompt Prompt::from_json(const json& j) {
        Prompt prompt;
        prompt.name = j["name"];
        if (j.contains("description")) prompt.description = j["description"];
        if (j.contains("arguments")) {
            for (const auto& arg : j["arguments"]) {
                prompt.arguments.push_back(PromptArgument::from_json(arg));
            }
        }
        return prompt;
    }

    // PromptMessage implementation
    json PromptMessage::to_json() const {
        json j;
        j["role"] = (role == Role::User) ? "user" : "assistant";
        j["content"] = content;
        return j;
    }

    PromptMessage PromptMessage::from_json(const json& j) {
        PromptMessage message;
        std::string role_str = j.at("role").get<std::string>();
        message.role = (role_str == "user") ? Role::User : Role::Assistant;
        message.content = j["content"];
        return message;
    }
}

#include "types.h"

namespace mcp {

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

    json ContentItem::to_json() const {
        json j;
        j["type"] = type;
        if (text) j["text"] = *text;
        if (base64) j["base64"] = *base64;
        if (mime_type) j["mime_type"] = *mime_type;
        if (uri) j["uri"] = *uri;
        return j;
    }

    ContentItem ContentItem::from_json(const json& j) {
        ContentItem item;
        item.type = j["type"];
        if (j.contains("text")) item.text = j["text"];
        if (j.contains("base64")) item.base64 = j["base64"];
        if (j.contains("mime_type")) item.mime_type = j["mime_type"];
        if (j.contains("uri")) item.uri = j["uri"];
        return item;
    }

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

    json Resources::to_json() const {
        json j;
        j["uri"] = uri;
        j["name"] = name;
        if (description) j["description"] = *description;
        if (mime_type) j["mime_type"] = *mime_type;
        return j;
    }

    Resources Resources::from_json(const json& j) {
        Resources resources;
        resources.uri = j["uri"];
        resources.name = j["name"];
        if (j.contains("description")) resources.description = j["description"];
        if (j.contains("mime_type")) resources.mime_type = j["mime_type"];
        return resources;
    }

    json ResourcesContent::to_json() const {
        json j = {
            {"uri", uri},
            {"text", text}
        };

        if (mime_type.has_value()) {
            j["mime_type"] = *mime_type;
        }
        if (base64.has_value()) {
            j["base64"] = *base64;
        }
        return j;
    }

    ResourcesContent ResourcesContent::from_json(const json& j) {
        ResourcesContent content;
        content.uri = j["uri"];
        if (j.contains("mime_type")) content.mime_type = j["mime_type"];
        content.text = j["text"];
        if (j.contains("base64")) content.base64 = j["base64"];
        return content;
    }

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

    json ServerCapabilities::ToolCapabilities::to_json() const {
        json j;
        j["list_changed"] = list_changed;
        return j;
    }

    ServerCapabilities::ToolCapabilities ServerCapabilities::ToolCapabilities::from_json(const json& j) {
        ServerCapabilities::ToolCapabilities capabilities;
        capabilities.list_changed = j.value("list_changed", false);
        return capabilities;
    }

    json ServerCapabilities::ResourcesCapabilities::to_json() const {
        json j;
        j["subscribed"] = subscribed;
        j["list_changed"] = list_changed;
        return j;
    }

    ServerCapabilities::ResourcesCapabilities ServerCapabilities::ResourcesCapabilities::from_json(const json& j) {
        ServerCapabilities::ResourcesCapabilities capabilities;
        capabilities.subscribed = j.value("subscribed", false);
        capabilities.list_changed = j.value("list_changed", false);
        return capabilities;
    }

    json ServerCapabilities::PromptCapabilities::to_json() const {
        json j;
        j["list_changed"] = list_changed;
        return j;
    }

    ServerCapabilities::PromptCapabilities ServerCapabilities::PromptCapabilities::from_json(const json& j) {
        ServerCapabilities::PromptCapabilities capabilities;
        capabilities.list_changed = j.value("list_changed", false);
        return capabilities;
    }

    json ServerCapabilities::to_json() const {
        json j;
        if (tool_capabilities) j["tool_capabilities"] = tool_capabilities->to_json();
        if (resources_capabilities) j["resources_capabilities"] = resources_capabilities->to_json();
        if (prompt_capabilities) j["prompt_capabilities"] = prompt_capabilities->to_json();
        if (logging) j["logging"] = *logging;
        return j;
    }

    ServerCapabilities ServerCapabilities::from_json(const json& j) {
        ServerCapabilities capabilities;
        if (j.contains("tool_capabilities")) {
            capabilities.tool_capabilities = ToolCapabilities::from_json(j["tool_capabilities"]);
        }
        if (j.contains("resources_capabilities")) {
            capabilities.resources_capabilities = ResourcesCapabilities::from_json(j["resources_capabilities"]);
        }
        if (j.contains("prompt_capabilities")) {
            capabilities.prompt_capabilities = PromptCapabilities::from_json(j["prompt_capabilities"]);
        }
        if (j.contains("logging")) {
            capabilities.logging = j["logging"];
        }
        return capabilities;
    }

    json ServerInfo::to_json() const {
        json j;
        j["name"] = name;
        j["version"] = version;
        return j;
    }

    ServerInfo ServerInfo::from_json(const json& j) {
        ServerInfo info;
        info.name = j["name"];
        info.version = j["version"];
        return info;
    }

    json InitializeResult::to_json() const {
        json j;
        j["version"] = version;
        j["capabilities"] = capabilities.to_json();
        j["info"] = info.to_json();
        return j;
    }

    InitializeResult InitializeResult::from_json(const json& j) {
        InitializeResult result;
        result.version = j["version"];
        result.capabilities = ServerCapabilities::from_json(j["capabilities"]);
        result.info = ServerInfo::from_json(j["info"]);
        return result;
    }
}

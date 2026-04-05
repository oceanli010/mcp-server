#include "config/config.h"
#include "logger/logger.h"
#include "auth/auth.h"
#include "http_jsonrpc.h"
#include "jsonrpc.h"
#include "mcp_server.h"

#include <curl/curl.h>

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <memory>
#include <ctime>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace mcp;

static std::atomic<bool> g_running(false);
static std::unique_ptr<HttpJsonRpcServer> g_http_server{nullptr};

void signal_handler(const int signum) {
    std::cerr << "\nReceived signal: " << signum << std::endl;
    g_running = false;
    if (g_http_server) {
        g_http_server->stop();
    }
}

spdlog::level::level_enum stringToLogLevel(const std::string& level_str) {
    if (level_str == "debug") {
        return spdlog::level::debug;
    } else if (level_str == "info") {
        return spdlog::level::info;
    } else if (level_str == "warn") {
        return spdlog::level::warn;
    } else if (level_str == "error") {
        return spdlog::level::err;
    } else if (level_str == "critical") {
        return spdlog::level::critical;
    } else if (level_str == "trace") {
        return spdlog::level::trace;
    }
    return spdlog::level::info;
}

void setup_mcp_server(McpServer& mcp) {
    {
        Tool tool;
        tool.name = "echo";
        tool.description = "Echo back the input message";
        tool.input_schema.properties = {
                {"message", {{"type", "string"}, {"description", "Message to echo"}}}
        };
        tool.input_schema.requirements = {"message"};

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            ToolResult result;
            result.content_items.push_back(ContentItem{
                .type ="text",
                .text = "Echo: " + args.at("message").get<std::string>()
            });
            return result;
        });
    }

    {
        Tool tool;
        tool.name ="calculate";
        tool.description = "Perform basic arithmetic operations";
        tool.input_schema.properties = {
            {"operation", {
                {"type", "string"},
                {"enum", json::array({"add", "subtract", "multiply", "divide"})}
            }},
            {"a", {{"type", "number"}}},
            {"b", {{"type", "number"}}}
        };
        tool.input_schema.requirements = {"operation", "a", "b"};

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            std::string operation = args.at("operation").get<std::string>();
            double a = args.at("a").get<double>();
            double b = args.at("b").get<double>();
            double result_val = 0;

            if (operation == "add") {
                result_val = a + b;
            } else if (operation == "subtract") {
                result_val = a - b;
            } else if (operation == "multiply") {
                result_val = a * b;
            } else if (operation == "divide") {
                if (b == 0) {
                    ToolResult error;
                    error.is_error = true;
                    error.content_items.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: Divide by zero"
                    });
                    return error;
                }
                result_val = a / b;
            }

            ToolResult result;
            result.content_items.push_back(ContentItem{
                .type ="text",
                .text = std::to_string(result_val)
            });
            return result;
        });
    }

    {
        Tool tool;
        tool.name = "get_time";
        tool.description = "Get the current time";
        tool.input_schema.properties = json::object();

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            std::time_t now = std::time(nullptr);
            char time_str[64];
            std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

            ToolResult result;
            result.content_items.push_back(ContentItem{
                .type ="text",
                .text = time_str
            });
            return result;
        });
    }

    {
        Tool tool;
        tool.name ="get_weather";
        tool.description = "Get weather information for a city";
        tool.input_schema.properties = {
            {"city", {{"type", "string"}, {"description", "City name (e.g., Beijing, Shanghai)"}}},
            {"api_key", {{"type", "string"}, {"description", "WeatherAPI.com API key (optional)"}}}
        };
        tool.input_schema.requirements = {"city"};

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            std::string city = args.at("city").get<std::string>();
            std::string api_key = args.value("api_key", "853851dfd755466fbf743931260404");

            // URL编码城市名称
            std::string encoded_city;
            for (char c : city) {
                if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                    encoded_city += c;
                } else {
                    char hex[4];
                    snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
                    encoded_city += hex;
                }
            }

            // 使用libcurl进行HTTP请求
            CURL* curl = curl_easy_init();
            if (!curl) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = "Error: Failed to initialize CURL"
                });
                return error;
            }

            // 使用WeatherAPI.com天气API
            std::string weather_url = "https://api.weatherapi.com/v1/current.json?key=" + api_key +
                                    "&q=" + encoded_city + "&aqi=no&lang=zh";
        
            std::string weather_response;
            curl_easy_setopt(curl, CURLOPT_URL, weather_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, std::string* data) {
                data->append(ptr, size * nmemb);
                return size * nmemb;
            });
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &weather_response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        
            if (res != CURLE_OK) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: HTTP request failed: ") + curl_easy_strerror(res)
                });
                return error;
            }

            // 解析天气数据
            json weather_data;
            try {
                weather_data = json::parse(weather_response);
            } catch (const std::exception& e) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: Failed to parse weather response: ") + e.what()
                });
                return error;
            }

            if (weather_data.contains("error")) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = "Error: " + weather_data["error"]["message"].get<std::string>()
                });
                return error;
            }

            if (!weather_data.contains("location") || !weather_data.contains("current")) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = "Error: Invalid weather data received"
                });
                return error;
            }

            auto& location = weather_data["location"];
            auto& current = weather_data["current"];
        
            std::string location_name = location["name"];
            std::string country = location["country"];
            double temperature = current["temp_c"];
            double apparent_temp = current["feelslike_c"];
            int humidity = current["humidity"];
            double wind_speed = current["wind_kph"];
            std::string wind_dir_str = current["wind_dir"];
            std::string weather_desc = current["condition"]["text"];

            // 构建天气信息字符串
            std::ostringstream oss;
            oss << "【" << location_name;
            if (!country.empty()) {
                oss << ", " << country;
            }
            oss << "天气信息】\n";
            oss << "天气状况: " << weather_desc << "\n";
            oss << "温度: " << std::fixed << std::setprecision(1) << temperature << "°C\n";
            oss << "体感温度: " << std::fixed << std::setprecision(1) << apparent_temp << "°C\n";
            oss << "湿度: " << humidity << "%\n";
            oss << "风速: " << std::fixed << std::setprecision(1) << wind_speed << " km/h\n";
            oss << "风向: " << wind_dir_str;

            ToolResult result;
            result.content_items.push_back(ContentItem{
                .type = "text",
                .text = oss.str()
            });
            return result;
        });
    }

    {
        Tool tool;
        tool.name = "write_file";
        tool.description = "Write content to a file";
        tool.input_schema.properties = {
            {"path", {{"type", "string"}, {"description", "File path to write to"}}},
            {"context", {{"type", "string"}, {"description", "Content to write to the file"}}}
        };
        tool.input_schema.requirements = {"path", "context"};

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            std::string path =args.at("path").get<std::string>();
            std::string context = args.at("context").get<std::string>();
            ToolResult result;

            try {
                std::ofstream file(path);
                if (!file.is_open()) {
                    result.is_error = true;
                    result.content_items.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: Failed to open file for writing"
                    });
                    return result;
                }

                file << context;
                file.close();
                result.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = "Successfully wrote to file: " + path
                });
            } catch (const std::exception& e) {
                result.is_error = true;
                result.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: ") + e.what()
                });
            }
            return result;
        });
    }

    {
        Resources resources;
        resources.uri = "system://info";
        resources.name = "System Information";
        resources.description = "Basic system information";
        resources.mime_type = "text/plain";

        mcp.register_resource(resources, [](const std::string& uri) -> ResourcesContent {
            ResourcesContent content;
            content.uri = uri;
            content.mime_type = "text/plain";

            std::ostringstream oss;
            oss << "MCP Server - System Info\n";
            oss << "========================\n";
            std::time_t now = std::time(nullptr);
            char buf[100];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
            oss << "Time: " << buf << "\n";

            content.text = oss.str();
            return content;
        });
    }

    {
        Resources resources;
        resources.uri = "config://server";
        resources.name = "Server Configuration";
        resources.mime_type = "application/json";

        mcp.register_resource(resources, [](const std::string& uri) -> ResourcesContent {
            ResourcesContent content;
            content.uri = uri;
            content.mime_type = "application/json";
            content.text = json({
                {"port", MCP_CONFIG.getServerPort()},
                {"log_level", MCP_CONFIG.getLogLevel()},
            }).dump(2);
            return content;
        });
    }

    {
        Prompt prompt;
        prompt.name = "code_review";
        prompt.description = "Generate code review prompt";
        prompt.arguments.push_back(PromptArgument{.name = "code", .required = true});
        prompt.arguments.push_back(PromptArgument{.name = "language", .required = true});

        mcp.register_prompt(prompt, [](const json& args) -> std::vector<PromptMessage> {
            std::vector<PromptMessage> msgs;
            PromptMessage msg;
            msg.role =Role::User;
            msg.content = {
                {"type", "text"},
                {"text", "Please review this " + args.at("language").get<std::string>() + " code:\n\n" + args.at("code").get<std::string>()}
            };
            msgs.push_back(msg);
            return msgs;
        });
    }

    MCP_LOG_INFO("MCP setup complete: {} tools, {} resources, {} prompts", mcp.list_tools().size(), mcp.list_resources().size(), mcp.list_prompts().size());
}

JsonRpcDispatcher create_dispatcher(McpServer& mcp_server) {
    JsonRpcDispatcher dispatcher;

    dispatcher.registerHandler("initialize", [&mcp_server](const json& params) -> json {
        MCP_LOG_INFO("Client initialized");
        return mcp_server.get_initialize_result().to_json();
    });

    dispatcher.registerHandler("tools/list", [&mcp_server](const json& params) -> json {
        json tools_arr = json::array();
        for (const auto& item : mcp_server.list_tools()) {
            tools_arr.push_back(item.to_json());
        }
        return {{"tools", tools_arr}};
    });

    dispatcher.registerHandler("tools/call", [&mcp_server](const json& params) -> json {
        std::string name = params.at("name").get<std::string>();
        json arguments = params.value("arguments", json{});
        MCP_LOG_INFO("Call tool: {}", name);
        auto result =mcp_server.call_tool(name, arguments);
        return result.to_json();
    });

    dispatcher.registerHandler("resources/list", [&mcp_server](const json& params) -> json {
        json resources_arr = json::array();
        for (const auto& item : mcp_server.list_resources()) {
            resources_arr.push_back(item.to_json());
        }
        return {{"resources", resources_arr}};
    });

    dispatcher.registerHandler("resources/read", [&mcp_server](const json& params) -> json {
        std::string uri = params.at("uri").get<std::string>();
        MCP_LOG_INFO("Read resource {}", uri);
        auto context =mcp_server.read_resources(uri);
        json contexts_arr = json::array();
        contexts_arr.push_back(context.to_json());
        return contexts_arr;
    });

    dispatcher.registerHandler("prompts/list", [&mcp_server](const json& params) -> json {
        json prompts_arr = json::array();
        for (const auto& item : mcp_server.list_prompts()) {
            prompts_arr.push_back(item.to_json());
        }
        return {{"prompts", prompts_arr}};
    } );

    dispatcher.registerHandler("prompts/get", [&mcp_server](const json& params) -> json {
        std::string name = params.at("name").get<std::string>();
        json arguments = params.value("arguments", json{});
        MCP_LOG_INFO("Getting prompt {}", name);
        auto messages =mcp_server.get_prompt(name, arguments);
        json messages_arr = json::array();
        for (const auto& item : messages) {
            messages_arr.push_back(item.to_json());
        }
        return {{"messages", messages_arr}};
    });

    return dispatcher;
}

void run_http_mode(McpServer& mcp_server, const std::string& host, int port) {
    MCP_LOG_INFO("Running http server: {}:{}", host, port);
    auto dispatcher = create_dispatcher(mcp_server);
    g_http_server = std::make_unique<HttpJsonRpcServer>(std::move(dispatcher), host, port);

    struct {
        std::vector<json> events;
        std::mutex mutex;
        std::condition_variable cv;
    } event_queue;

    mcp_server.set_sse_callback([&event_queue](const json& event) {
        std::lock_guard<std::mutex> lock(event_queue.mutex);
        event_queue.events.push_back(event);
        event_queue.cv.notify_all();
    });

    g_http_server->registerSseEndPoint("sse/events", [&mcp_server](const auto& send) {
        MCP_LOG_INFO("SSE events client connected");
        send(json({{"type", "connected"}, {"message", "Server events stream"}}).dump());

        int count = 0;
        while (g_running.load()) {
            json status = {
                {"type", "server_status"},
                {"timestamp", std::time(nullptr)},
                {"tools_count",mcp_server.list_tools().size()},
                {"resources_count", mcp_server.list_resources().size()},
                {"prompts_count", mcp_server.list_prompts().size()},
                {"uptime_sec", count++}
            };
            send(status.dump());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        MCP_LOG_INFO("SSE events client disconnected");
    });

    g_http_server->registerSseEndPoint("sse/tool_calls", [&event_queue](const auto& send) {
        MCP_LOG_INFO("SSE events client connected");
        send(json({{"type", "connected"}, {"message", "Tool calls monitoring"}}).dump());
        while (g_running.load()) {
            std::unique_lock<std::mutex> lock(event_queue.mutex);

            event_queue.cv.wait_for(lock, std::chrono::seconds(1), [&event_queue] {
                return !event_queue.events.empty();
            });

            for (const auto& item : event_queue.events) {
                send(item.dump());
            }
            event_queue.events.clear();
        }
        MCP_LOG_INFO("SSE tool_calls client disconnected");
    });

    g_http_server->run();

    MCP_LOG_INFO("HTTP server stopped");
}

void run_stdio_mode(McpServer& mcp_server) {
    MCP_LOG_INFO("Starting stdio server");

    auto dispatcher = create_dispatcher(mcp_server);
    StdioJsonRpcServer server(std::move(dispatcher));
    server.run();

    MCP_LOG_INFO("stdio server stopped");
}

void run_both_mode(McpServer& mcp_server, const std::string& host, int port) {
    MCP_LOG_INFO("Starting both HTTP and stdio servers");
    std::thread http_thread([&mcp_server, &host, port] {
        run_http_mode(mcp_server, host, port);
    });

    run_stdio_mode(mcp_server);

    if (http_thread.joinable()) {
        http_thread.join();
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = "../../config/server.json";
    std::string mode = "http";
    std::string host;
    int port = 0;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        }
    }

    if (mode != "http" && mode != "stdio" && mode != "both") {
        std::cerr << "Invalid mode: " << mode << " (must be http, stdio, or both)" << std::endl;
        return 1;
    }

    if (!MCP_CONFIG.loadConfigFile(config_file)) {
        std::cerr << "Failed to load config file: " << config_file << std::endl;
        return 1;
    }

    if (host.empty()) {
        host = "0.0.0.0";
    }

    if (port == 0) {
        port = MCP_CONFIG.getServerPort();
    }

    MCP_LOG_INIT(
        "mcp_server",
        MCP_CONFIG.getLogPath(),
        MCP_CONFIG.getLogFileSize(),
        MCP_CONFIG.getLogFileCount(),
        MCP_CONFIG.getLogConsoleOutput()
    );
    MCP_LOG_SET_LEVEL(stringToLogLevel(MCP_CONFIG.getLogLevel()));

    auto api_keys = MCP_CONFIG.getApiKeys();
    MCP_AUTH.init(api_keys);

    MCP_LOG_INFO("Starting MCP Server");
    MCP_LOG_INFO("Config file: {}", config_file);
    MCP_LOG_INFO("Mode: {}", mode);
    if (mode == "http" || mode == "both") {
        MCP_LOG_INFO("HTTP: {}:{}", host, port);
    }
    MCP_LOG_INFO("Authentication: {}", MCP_AUTH.isEnabled() ? "Enabled" : "Disabled");

    try {
        McpServer mcp_server("mcp-server", "1.0.0");

        ServerCapabilities server_capabilities;
        server_capabilities.tool_capabilities = ServerCapabilities::ToolCapabilities{false};
        server_capabilities.resources_capabilities = ServerCapabilities::ResourcesCapabilities{false, false};
        server_capabilities.prompt_capabilities = ServerCapabilities::PromptCapabilities{false};
        mcp_server.set_capabilities(server_capabilities);

        setup_mcp_server(mcp_server);

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        if (mode == "http") {
            run_http_mode(mcp_server, host, port);
        } else if (mode == "stdio") {
            run_stdio_mode(mcp_server);
        } else if (mode == "both") {
            run_both_mode(mcp_server, host, port);
        }

        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (g_http_server) {
            g_http_server->stop();
        }

        MCP_LOG_INFO("Server shutdown complete");
    } catch (const std::exception& e) {
        MCP_LOG_ERROR("Exception: {}", e.what());
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    MCP_LOG_FLUSH();
    MCP_LOG_SHUTDOWN();

    MCP_LOG_INFO("MCP Server stopped");

    return 0;
}
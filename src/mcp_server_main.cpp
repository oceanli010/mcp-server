///MCP Server主程序

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
#include <vector>
#include <string>
#include <stdexcept>

using namespace mcp;

//全局服务器实例指针
static std::atomic<bool> g_running(false);
static std::unique_ptr<HttpJsonRpcServer> g_http_server{nullptr};

//信号处理，用于接收关闭信号
void signal_handler(const int signum) {
    std::cerr << "\nReceived signal: " << signum << std::endl;
    g_running = false;
    if (g_http_server) {
        g_http_server->stop();
    }
}

//字符串转换日志级别
spdlog::level::level_enum stringToLogLevel(const std::string& level_str) {
    static const std::unordered_map<std::string, spdlog::level::level_enum> level_map = {
        {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},
        {"critical", spdlog::level::critical},
        {"trace", spdlog::level::trace}
    };
    auto it = level_map.find(level_str);
    return it != level_map.end() ? it->second : spdlog::level::info;
}

//注册MCP Server工具、资源、提示词
void setup_mcp_server(McpServer& mcp) {
    //Echo工具
    {
        Tool tool;
        tool.name = "echo";
        tool.description = "Echo back the input message";
        tool.input_schema.properties = {
                {"message", {{"type", "string"}, {"description", "Message to echo"}}}
        };
        tool.input_schema.requirements = {"message"};

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            try {
                ToolResult result;
                result.content_items.push_back(ContentItem{
                    .type ="text",
                    .text = "Echo: " + args.at("message").get<std::string>()
                });
                return result;
            } catch (const std::exception& e) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: ") + e.what()
                });
                return error;
            }
        });
    }

    //计算器
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
            try {
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
                } else {
                    ToolResult error;
                    error.is_error = true;
                    error.content_items.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: Invalid operation"
                    });
                    return error;
                }

                ToolResult result;
                result.content_items.push_back(ContentItem{
                    .type ="text",
                    .text = std::to_string(result_val)
                });
                return result;
            } catch (const std::exception& e) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: ") + e.what()
                });
                return error;
            }
        });
    }

    //时钟
    {
        Tool tool;
        tool.name = "get_time";
        tool.description = "Get the current time";
        tool.input_schema.properties = json::object();

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            try {
                std::time_t now = std::time(nullptr);
                char time_str[64];
                std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

                ToolResult result;
                result.content_items.push_back(ContentItem{
                    .type ="text",
                    .text = time_str
                });
                return result;
            } catch (const std::exception& e) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: ") + e.what()
                });
                return error;
            }
        });
    }

    //天气获取
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
            try {
                //提取参数信息
                std::string city = args.at("city").get<std::string>();
                std::string api_key = args.value("api_key", "853851dfd755466fbf743931260404");

                //城市名编码
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

                //CURL初始化
                struct CurlGuard {
                    CURL* curl;
                    CurlGuard(CURL* c) : curl(c) {}
                    ~CurlGuard() { if (curl) curl_easy_cleanup(curl); }
                };

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
                CurlGuard guard(curl);

                //构建请求URL
                std::string weather_url = "https://api.weatherapi.com/v1/current.json?key=" + api_key +
                                        "&q=" + encoded_city + "&aqi=no&lang=zh";

                //配置CURL请求
                std::string weather_response;
                curl_easy_setopt(curl, CURLOPT_URL, weather_url.c_str());   //设置请求URL
                //设置写入函数
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, std::string* data) {
                    data->append(ptr, size * nmemb);
                    return size * nmemb;
                });
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &weather_response);   //响应信息保存到 weather_response
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);       //10s超时时间
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L); //SSL验证启用
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L); //验证主机名
                //使用User-Agent头模拟浏览器请求
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");

                CURLcode res = curl_easy_perform(curl);         //执行请求

                //检查错误状态
                if (res != CURLE_OK) {
                    ToolResult error;
                    error.is_error = true;
                    error.content_items.push_back(ContentItem{
                        .type = "text",
                        .text = std::string("Error: HTTP request failed: ") + curl_easy_strerror(res)
                    });
                    return error;
                }

                //解析json响应
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

                //错误检查
                if (weather_data.contains("error")) {
                    ToolResult error;
                    error.is_error = true;
                    error.content_items.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: " + weather_data["error"]["message"].get<std::string>()
                    });
                    return error;
                }

                //验证数据结构
                if (!weather_data.contains("location") || !weather_data.contains("current")) {
                    ToolResult error;
                    error.is_error = true;
                    error.content_items.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: Invalid weather data received"
                    });
                    return error;
                }

                //提取天气信息
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

                //构建输出字符串
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
            } catch (const std::exception& e) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: ") + e.what()
                });
                return error;
            }
        });
    }

    //文件写入
    {
        Tool tool;
        tool.name = "write_file";
        tool.description = "Write content to a file";
        tool.input_schema.properties = {
            {"path", {{"type", "string"}, {"description", "File path to write to"}}},
            {"content", {{"type", "string"}, {"description", "Content to write to the file"}}}
        };
        tool.input_schema.requirements = {"path", "content"};

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            try {
                std::string path = args.at("path").get<std::string>();
                std::string content = args.at("content").get<std::string>();
                ToolResult result;

                if (path.find("../") != std::string::npos) {
                    result.is_error = true;
                    result.content_items.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: Invalid file path"
                    });
                    return result;
                }

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

                    file << content;
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
            } catch (const std::exception& e) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: ") + e.what()
                });
                return error;
            }
        });
    }

    //获取cpu信息
    {
        Tool tool;
        tool.name = "get_cpu_info";
        tool.description = "Get CPU resource information";
        tool.input_schema.properties = json::object();

        mcp.register_tool(tool, [](const json& args) -> ToolResult {
            try {
                ToolResult result;
                std::ostringstream oss;

                //通过 /proc 目录获取cpu信息
                std::ifstream cpuinfo("/proc/cpuinfo");
                if (cpuinfo.is_open()) {
                    std::string line;
                    while (std::getline(cpuinfo, line)) {
                        if (line.find("model name") == 0) {
                            oss << "CPU Model: " << line.substr(line.find(":") + 2) << "\n";
                            break;
                        }
                    }
                    cpuinfo.close();
                }

                //计算核心数
                std::ifstream stat("/proc/stat");
                if (stat.is_open()) {
                    int core_count = 0;
                    std::string line;
                    while (std::getline(stat, line)) {
                        if (line.substr(0, 3) == "cpu" && line.length() > 3 && isdigit(line[3])) {
                            core_count++;
                        }
                    }
                    //stat.close();
                    oss << "CPU Cores: " << core_count << "\n";
                }

                //计算cpu使用率
                //std::ifstream stat_file("/proc/stat");
                if (stat.is_open()) {
                    std::string line;
                    std::getline(stat, line);
                    stat.close();

                    std::istringstream iss(line);
                    std::string cpu; 
                    long user, nice, system, idle, iowait, irq, softirq;
                    iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

                    long total = user + nice + system + idle + iowait + irq + softirq;
                    double usage = 100.0 * (total - idle) / total;
                    oss << "CPU Usage: " << std::fixed << std::setprecision(2) << usage << "%\n";
                }

                //计算cpu温度
                std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
                if (temp_file.is_open()) {
                    int temp;
                    temp_file >> temp;
                    temp_file.close();
                    double temp_c = temp / 1000.0;
                    oss << "CPU Temperature: " << std::fixed << std::setprecision(1) << temp_c << "°C\n";
                }

                //获取平均负载
                std::ifstream loadavg("/proc/loadavg");
                if (loadavg.is_open()) {
                    double load1, load5, load15;
                    loadavg >> load1 >> load5 >> load15;
                    loadavg.close();
                    oss << "Load Average (1/5/15 min): " << load1 << "/" << load5 << "/" << load15 << "\n";
                }

                result.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = oss.str()
                });
                return result;
            } catch (const std::exception& e) {
                ToolResult error;
                error.is_error = true;
                error.content_items.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error: ") + e.what()
                });
                return error;
            }
        });
    }

    //可以在这里添加更多工具

    //系统信息
    {
        Resources resources;
        resources.uri = "system://info";
        resources.name = "System Information";
        resources.description = "Basic system information";
        resources.mime_type = "text/plain";

        mcp.register_resource(resources, [](const std::string& uri) -> ResourcesContent {
            try {
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
            } catch (const std::exception& e) {
                ResourcesContent content;
                content.uri = uri;
                content.mime_type = "text/plain";
                content.text = std::string("Error: ") + e.what();
                return content;
            }
        });
    }

    //配置信息
    {
        Resources resources;
        resources.uri = "config://server";
        resources.name = "Server Configuration";
        resources.mime_type = "application/json";

        mcp.register_resource(resources, [](const std::string& uri) -> ResourcesContent {
            try {
                ResourcesContent content;
                content.uri = uri;
                content.mime_type = "application/json";
                content.text = json({
                    {"port", MCP_CONFIG.getServerPort()},
                    {"log_level", MCP_CONFIG.getLogLevel()},
                }).dump(2);
                return content;
            } catch (const std::exception& e) {
                ResourcesContent content;
                content.uri = uri;
                content.mime_type = "application/json";
                json error_json; error_json["error"] = e.what(); content.text = error_json.dump(2);
                return content;
            }
        });
    }

    //代码审查
    {
        Prompt prompt;
        prompt.name = "code_review";
        prompt.description = "Generate code review prompt";
        prompt.arguments.push_back(PromptArgument{.name = "code", .required = true});
        prompt.arguments.push_back(PromptArgument{.name = "language", .required = true});

        mcp.register_prompt(prompt, [](const json& args) -> std::vector<PromptMessage> {
            try {
                std::vector<PromptMessage> msgs;
                PromptMessage msg;
                msg.role = Role::User;
                msg.content = {
                    {"type", "text"},
                    {"text", "Please review this " + args.at("language").get<std::string>() + " code:\n\n" + args.at("code").get<std::string>()}
                };
                msgs.push_back(msg);
                return msgs;
            } catch (const std::exception& e) {
                std::vector<PromptMessage> msgs;
                PromptMessage msg;
                msg.role = Role::User;
                msg.content = {
                    {"type", "text"},
                    {"text", std::string("Error: ") + e.what()}
                };
                msgs.push_back(msg);
                return msgs;
            }
        });
    }

    MCP_LOG_INFO("MCP setup complete: {} tools, {} resources, {} prompts", mcp.list_tools().size(), mcp.list_resources().size(), mcp.list_prompts().size());
}

// 创建 JSON-RPC 调度器（绑定到 MCP 服务器）
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

//HTTP模式
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

//stdio模式
void run_stdio_mode(McpServer& mcp_server) {
    MCP_LOG_INFO("Starting stdio server");

    auto dispatcher = create_dispatcher(mcp_server);
    StdioJsonRpcServer server(std::move(dispatcher));
    server.run();

    MCP_LOG_INFO("stdio server stopped");
}

//同时运行
void run_both_mode(McpServer& mcp_server, const std::string& host, int port) {
    MCP_LOG_INFO("Starting both HTTP and stdio servers");
    std::thread http_thread;
    try {
        http_thread = std::thread([&mcp_server, &host, port] {
            run_http_mode(mcp_server, host, port);
        });
        
        run_stdio_mode(mcp_server);
        
        if (http_thread.joinable()) {
            http_thread.join();
        }
    } catch (const std::exception& e) {
        MCP_LOG_ERROR("Exception in both mode: {}", e.what());
        if (http_thread.joinable()) {
            http_thread.join();
        }
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = "../../config/server.json"; //配置文件
    std::string mode = "http";  //默认http模式
    std::string host;
    int port = 0;

    //命令行参数解析
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

    //模式读取
    if (mode != "http" && mode != "stdio" && mode != "both") {
        std::cerr << "Invalid mode: " << mode << " (must be http, stdio, or both)" << std::endl;
        return 1;
    }

    //加载配置文件
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

    //日志初始化
    MCP_LOG_INIT(
        "mcp_server",
        MCP_CONFIG.getLogPath(),
        MCP_CONFIG.getLogFileSize(),
        MCP_CONFIG.getLogFileCount(),
        MCP_CONFIG.getLogConsoleOutput()
    );
    MCP_LOG_SET_LEVEL(stringToLogLevel(MCP_CONFIG.getLogLevel()));

    //获取认证API_KEY
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
        //创建MCP实例
        McpServer mcp_server("mcp-server", "1.0.0");

        //设置服务器能力
        ServerCapabilities server_capabilities;
        server_capabilities.tool_capabilities = ServerCapabilities::ToolCapabilities{false};
        server_capabilities.resources_capabilities = ServerCapabilities::ResourcesCapabilities{false, false};
        server_capabilities.prompt_capabilities = ServerCapabilities::PromptCapabilities{false};
        mcp_server.set_capabilities(server_capabilities);

        setup_mcp_server(mcp_server);

        //信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        //按模式启动服务器
        if (mode == "http") {
            run_http_mode(mcp_server, host, port);
        } else if (mode == "stdio") {
            run_stdio_mode(mcp_server);
        } else if (mode == "both") {
            run_both_mode(mcp_server, host, port);
        }

        //等待中止信号
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
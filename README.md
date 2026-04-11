# MCP Server 项目文档

## 项目概述

MCP (Model Context Protocol) Server 是一个轻量级的工具调用服务，为大语言模型提供本地工具能力。它允许模型通过 JSON-RPC 协议调用各种本地工具，如获取系统信息、执行数据库操作、获取天气信息等。

本项目包含服务器端和客户端两部分：
- 服务器端：基于 C++ 实现的 JSON-RPC 服务，提供工具注册和调用功能
- 客户端：基于 Python 实现的 Ollama 集成客户端，支持与大语言模型的交互

## 功能特性

### 核心功能

- **工具管理**：注册、列出和调用各种本地工具
- **JSON-RPC 接口**：提供标准的 JSON-RPC 2.0 接口，便于与各种客户端集成
- **Ollama 集成**：通过 Python 客户端与 Ollama 大语言模型无缝集成
- **多工具支持**：内置多种实用工具，如时间查询、天气查询、CPU信息获取等
- **对话管理**：支持多轮对话，保持上下文连贯性

### 内置工具

- `get_time`：获取当前时间
- `get_weather`：获取指定城市的天气信息
- `get_cpu_info`：获取CPU资源信息
- `calculate`：执行基本算术运算
- `echo`：回显输入消息
- `write_file`：写入内容到文件
- `mysql_connect`：连接到MySQL数据库
- `mysql_query`：执行MySQL查询
- `mysql_insert`：插入数据到MySQL表
- `mysql_update`：更新MySQL表中的数据

## 安装步骤

### 环境要求

- C++17 或更高版本
- CMake 3.16 或更高版本
- MySQL 客户端库（用于数据库工具）
- Python 3.7 或更高版本（用于客户端）
- Ollama（用于大语言模型集成）

### 服务器安装

1. **克隆代码库**

   ```bash
   git clone <repository-url>
   cd mcp_server
   ```

2. **构建项目**

   ```bash
   # 使用提供的构建脚本
   ./build.sh
   
   # 或手动构建
   mkdir -p build
   cd build
   cmake ..
   make -j4
   ```

3. **配置服务器**

   编辑 `config/server.json` 文件，根据需要修改配置：

   ```json
   {
     "server": {
       "port": 8089
     },
     "logging": {
       "log_file_path": "server.log",
       "log_level": "debug",
       "log_file_size": 52428800,
       "log_file_count": 5,
       "log_console_output": true
     }
   }
   ```

4. **启动服务器**

   ```bash
   ./build/src/mcp_server --config config/server.json
   ```

### 客户端安装

1. **安装依赖**

   ```bash
   pip install requests
   ```

2. **安装 Ollama**

   请参考 [Ollama 官方文档](https://ollama.ai/download) 安装 Ollama。

3. **下载模型**

   ```bash
   ollama pull qwen2.5:1.5b
   ```

## 使用指南

### 服务器使用

1. **启动服务器**

   ```bash
   ./build/src/mcp_server --config config/server.json
   ```

2. **验证服务器运行状态**

   服务器启动后，会在指定端口（默认为 8089）监听 JSON-RPC 请求。

### 客户端使用

1. **启动客户端**

   ```bash
   python client/mcp_ollama_client.py
   ```

2. **交互式使用**

   客户端启动后，会进入交互式模式，您可以输入问题并获得回答：

   ```
   MCP-Ollama Client
   Using model: qwen2.5:1.5b
   MCP Server: http://localhost:8089/jsonrpc
   Available tools: 10
   - mysql_update: Update data in MySQL table
   - mysql_insert: Insert data into MySQL table
   - mysql_query: Execute MySQL query
   - get_cpu_info: Get CPU resource information
   - write_file: Write content to a file
   - get_weather: Get weather information for a city
   - mysql_connect: Connect to MySQL database
   - get_time: Get the current time
   - calculate: Perform basic arithmetic operations
   - echo: Echo back the input message

   Commands:
   - exit: 退出程序
   - clear: 清除对话历史
   - tools: 显示可用工具

   You: 北京天气如何
   Assistant: 北京今天的天气是晴天，温度为21.2摄氏度，体感温度也保持在这个数值。湿度约为25%，风速为16.9 km/h，风向为南西偏南方向。
   ```

3. **命令说明**

   - `exit`：退出程序
   - `clear`：清除对话历史
   - `tools`：显示可用工具列表

## 配置说明

### 服务器配置

服务器配置文件位于 `config/server.json`，主要配置项包括：

| 配置项 | 说明 | 默认值 |
|-------|------|-------|
| `server.port` | 服务器监听端口 | 8089 |
| `logging.log_file_path` | 日志文件路径 | server.log |
| `logging.log_level` | 日志级别 | debug |
| `logging.log_file_size` | 单个日志文件大小限制（字节） | 52428800 |
| `logging.log_file_count` | 日志文件保留数量 | 5 |
| `logging.log_console_output` | 是否在控制台输出日志 | true |

### 客户端配置

客户端配置可以在 `mcp_ollama_client.py` 文件中修改：

```python
class MCPOllamaClient:
    def __init__(self, mcp_url="http://localhost:8089/jsonrpc", mcp_api_key=None, 
                 ollama_url="http://localhost:11434/api", ollama_model="qwen2.5:1.5b"):
        # 初始化客户端
```

主要配置参数：

| 参数 | 说明 | 默认值 |
|------|------|-------|
| `mcp_url` | MCP Server 的 JSON-RPC 接口地址 | http://localhost:8089/jsonrpc |
| `mcp_api_key` | MCP Server 的 API 密钥（如果需要） | None |
| `ollama_url` | Ollama API 地址 | http://localhost:11434/api |
| `ollama_model` | 使用的 Ollama 模型 | qwen2.5:1.5b |

## 常见问题解答 (FAQ)

### Q: 服务器启动失败，提示无法创建日志目录

**A:** 这通常是因为文件系统权限问题。请确保服务器有权限在指定的日志目录中创建文件，或者修改 `config/server.json` 中的 `log_file_path` 为相对路径（如 "server.log"）。

### Q: 客户端无法连接到 MCP Server

**A:** 请检查以下几点：
1. MCP Server 是否正在运行
2. 网络连接是否正常
3. 客户端配置的 `mcp_url` 是否正确
4. 防火墙是否阻止了连接

### Q: 工具调用失败

**A:** 工具调用失败可能有以下原因：
1. 工具参数不正确
2. 工具依赖的服务不可用（如 MySQL 数据库）
3. 工具执行过程中出现错误

请检查客户端输出的错误信息，了解具体失败原因。

### Q: 如何添加自定义工具

**A:** 要添加自定义工具，您需要：
1. 在服务器端注册新工具
2. 确保工具实现符合 MCP Server 的工具接口规范
3. 重启服务器后，客户端会自动发现新工具

## 贡献指南

### 代码风格

- C++ 代码：遵循 Google C++ 风格指南
- Python 代码：遵循 PEP 8 风格指南

### 提交规范

提交代码时，请使用清晰的提交信息，格式为：

```
<类型>: <描述>

<详细说明>
```

类型包括：
- `feat`：新功能
- `fix`：修复 bug
- `docs`：文档更新
- `style`：代码风格调整
- `refactor`：代码重构
- `test`：测试相关
- `chore`：其他变更

### 开发流程

1. Fork 代码库
2. 创建功能分支
3. 实现功能或修复 bug
4. 编写测试
5. 提交代码
6. 创建 Pull Request

### 问题报告

如果您发现问题或有功能建议，请在 GitHub Issues 中提交。提交时请包含：
- 问题描述
- 复现步骤
- 预期行为
- 实际行为
- 环境信息

## 许可证

本项目采用 MIT 许可证。详情请查看 LICENSE 文件。

## 联系方式

- 项目地址：<repository-url>
- 问题反馈：<repository-url>/issues

---

感谢您使用 MCP Server 项目！
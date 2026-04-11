import json
import requests

class MCPClient:
    def __init__(self, url="http://localhost:8089/jsonrpc", api_key=None):
        self.url = url
        self.api_key = api_key
        self.headers = {
            "Content-Type": "application/json"
        }
        if api_key:
            self.headers["X-API-Key"] = api_key
    
    def _send_request(self, method, params):
        payload = {
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
            "id": 1
        }
        response = requests.post(self.url, headers=self.headers, json=payload)
        response.raise_for_status()
        result = response.json()
        if "error" in result:
            raise Exception(f"MCP Error: {result['error']['message']}")
        return result.get("result", {})
    
    def list_tools(self):
        return self._send_request("tools/list", {})
    
    def call_tool(self, tool_name, arguments):
        params = {
            "name": tool_name,
            "arguments": arguments
        }
        return self._send_request("tools/call", params)
    
    def list_resources(self):
        return self._send_request("resources/list", {})
    
    def read_resource(self, uri):
        params = {
            "uri": uri
        }
        return self._send_request("resources/read", params)
    
    def list_prompts(self):
        return self._send_request("prompts/list", {})
    
    def get_prompt(self, prompt_name, arguments):
        params = {
            "name": prompt_name,
            "arguments": arguments
        }
        return self._send_request("prompts/get", params)

class OllamaClient:
    def __init__(self, url="http://localhost:11434/api", model="qwen2.5:1.5b"):
        self.url = url
        self.model = model
    
    def generate(self, prompt, tools=None, tool_choice="auto", format=None):
        """
        生成响应，支持工具使用和结构化输出
        """
        payload = {
            "model": self.model,
            "prompt": prompt,
            "stream": False
        }
        
        if tools:
            payload["tools"] = tools
            payload["tool_choice"] = tool_choice
        
        if format:
            payload["format"] = format
        
        response = requests.post(f"{self.url}/generate", json=payload)
        response.raise_for_status()
        return response.json()
    
    def chat(self, messages, tools=None, tool_choice="auto"):
        """
        聊天模式，支持多轮对话和工具使用
        """
        payload = {
            "model": self.model,
            "messages": messages,
            "stream": False
        }
        
        if tools:
            payload["tools"] = tools
            payload["tool_choice"] = tool_choice
        
        response = requests.post(f"{self.url}/chat", json=payload)
        response.raise_for_status()
        return response.json()

class MCPOllamaClient:
    def __init__(self, mcp_url="http://localhost:8089/jsonrpc", mcp_api_key=None, 
                 ollama_url="http://localhost:11434/api", ollama_model="qwen2.5:1.5b"):
        self.mcp_client = MCPClient(mcp_url, mcp_api_key)
        self.ollama_client = OllamaClient(ollama_url, ollama_model)
        self.tools = self._get_mcp_tools()
        self.conversation_history = []
    
    def _get_mcp_tools(self):
        """
        从MCP服务器获取工具列表，并转换为Ollama工具格式
        """
        try:
            mcp_tools = self.mcp_client.list_tools()
            ollama_tools = []
            
            for tool in mcp_tools.get("tools", []):
                tool_schema = {
                    "type": "function",
                    "function": {
                        "name": tool["name"],
                        "description": tool["description"],
                        "parameters": {
                            "type": "object",
                            "properties": {},
                            "required": tool["input_schema"].get("requirements", [])
                        }
                    }
                }
                
                # 处理输入参数
                for param_name, param_schema in tool["input_schema"].get("properties", {}).items():
                    tool_schema["function"]["parameters"]["properties"][param_name] = param_schema
                
                ollama_tools.append(tool_schema)
            
            return ollama_tools
        except Exception as e:
            print(f"无法获取MCP工具列表: {e}")
            print("继续运行，但将无法使用MCP工具")
            return []
    
    def process_query(self, query):
        """
        处理用户查询，使用Ollama模型和MCP工具
        """
        # 构建消息列表，包含系统提示和对话历史
        messages = [
            {
                "role": "system",
                "content": "你是一个智能助手，能够使用MCP Server提供的工具来回答用户问题。当用户的问题需要使用工具时，请调用相应的工具。\n\n必须使用工具的情况：\n1. 当用户询问当前时间时，使用get_time工具\n2. 当用户要求计算数学表达式时，使用calculate工具\n3. 当用户询问天气情况时，使用get_weather工具\n4. 当用户要求echo某个消息时，使用echo工具\n5. 当用户要求写入文件时，使用write_file工具\n6. 当用户要求连接MySQL数据库时，使用mysql_connect工具\n7. 当用户要求执行MySQL查询时，使用mysql_query工具\n8. 当用户要求插入数据到MySQL时，使用mysql_insert工具\n9. 当用户要求更新MySQL数据时，使用mysql_update工具\n10. 当用户询问CPU信息时，使用get_cpu_info工具\n\n请严格按照工具的参数要求进行调用，确保参数格式正确。"
            }
        ]
        
        # 添加对话历史
        messages.extend(self.conversation_history)
        
        # 添加当前用户查询
        messages.append({
            "role": "user",
            "content": query
        })
        
        # 第一次调用Ollama，可能会返回工具调用请求
        try:
            response = self.ollama_client.chat(messages, tools=self.tools)
            
            # 获取助手消息
            assistant_message = response.get("message", {})
            
            # 检查是否需要调用工具
            if "tool_calls" in assistant_message:
                tool_calls = assistant_message["tool_calls"]
                
                # 将助手的工具调用请求添加到消息中
                messages.append({
                    "role": "assistant",
                    "content": None,
                    "tool_calls": tool_calls
                })
                
                # 处理工具调用
                for tool_call in tool_calls:
                    tool_name = tool_call["function"]["name"]
                    arguments = tool_call["function"].get("arguments", {})
                    
                    # 调用MCP工具
                    try:
                        tool_result = self.mcp_client.call_tool(tool_name, arguments)
                        
                        # 将工具结果添加到消息中
                        messages.append({
                            "role": "tool",
                            "content": json.dumps(tool_result, ensure_ascii=False),
                            "tool_call_id": tool_call["id"]
                        })
                    except Exception as e:
                        # 将错误信息添加到消息中
                        messages.append({
                            "role": "tool",
                            "content": f"Error: {str(e)}",
                            "tool_call_id": tool_call["id"]
                        })
                
                # 再次调用Ollama，传递工具结果
                final_response = self.ollama_client.chat(messages, tools=self.tools)
                
                # 提取最终回答
                final_answer = final_response.get("message", {}).get("content", "")
                
                # 更新对话历史
                self.conversation_history.append({"role": "user", "content": query})
                self.conversation_history.append({"role": "assistant", "content": final_answer})
                
                return final_answer
            else:
                # 不需要调用工具，直接返回Ollama的回答
                final_answer = assistant_message.get("content", "")
                
                # 更新对话历史
                self.conversation_history.append({"role": "user", "content": query})
                self.conversation_history.append({"role": "assistant", "content": final_answer})
                
                return final_answer
        except Exception as e:
            return f"抱歉，处理您的请求时出错: {str(e)}"
    
    def interactive_mode(self):
        """
        交互式模式，用户可以输入问题，系统使用Ollama和MCP工具回答
        """
        print("MCP-Ollama Client")
        print(f"Using model: {self.ollama_client.model}")
        print(f"MCP Server: {self.mcp_client.url}")
        print(f"Available tools: {len(self.tools)}")
        for tool in self.tools:
            print(f"- {tool['function']['name']}: {tool['function']['description']}")
        print()
        print("Commands:")
        print("- exit: 退出程序")
        print("- clear: 清除对话历史")
        print("- tools: 显示可用工具")
        print()
        
        while True:
            try:
                query = input("You: ")
                if query.lower() == "exit":
                    break
                elif query.lower() == "clear":
                    self.conversation_history = []
                    print("对话历史已清除")
                    print()
                    continue
                elif query.lower() == "tools":
                    print("Available tools:")
                    for tool in self.tools:
                        print(f"- {tool['function']['name']}: {tool['function']['description']}")
                    print()
                    continue
                
                response = self.process_query(query)
                print(f"Assistant: {response}")
                print()
            except Exception as e:
                print(f"Error: {e}")
                print()

if __name__ == "__main__":
    client = MCPOllamaClient()
    client.interactive_mode()
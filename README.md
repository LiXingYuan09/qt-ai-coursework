# Qt AI Coursework

本仓库包含程序设计实训知识补强的三次 Qt 与 AI 综合作业示例工程：

1. `01-smart-login`：账号密码、本地人脸特征接口、SQLite 登录日志。
2. `02-ai-chat`：OpenAI 兼容接口客户端、SSE 流式回复和聊天界面。
3. `03-multimodal-rag`：本地文本分块、向量检索、RAG 上下文和工具路由。

## 环境

- Qt 6.5 或更高版本
- CMake 3.21 或更高版本
- C++17 编译器
- 模型文件、API 密钥、人脸样本和测试数据库不上传公开仓库。

## 构建

每个子目录均为独立 CMake 工程。例如：

```bash
cmake -S 02-ai-chat -B build/ai-chat
cmake --build build/ai-chat --config Release
```

课程演示代码仅用于学习，真实产品应使用专业口令派生函数、活体检测、系统密钥库和完善的权限控制。

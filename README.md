## 项目简介

这是一个基于C++实现的分布式系统，包含：
1. **聊天室系统**：支持多用户实时聊天、昵称设置、在线用户查看
2. **分布式文件存储系统**：支持文件存储、检索、删除和负载均衡

## 主要技术特性

### 聊天室系统
- 基于epoll的ET（边缘触发）模式
- Reactor模式事件驱动架构
- 使用vector高效管理客户端连接
- 支持TCP粘包处理（长度字段方式）
- 支持命令系统（/nick, /users, /help, /quit）

### 分布式文件存储系统
- 主从架构设计
- LRU缓存优化
- 多线程处理高并发
- 数据副本机制
- 负载均衡策略

## 编译安装

### 环境要求
- Linux系统
- GCC 4.8+ 或 Clang 3.3+
- CMake 3.10+

### 编译步骤

```bash
# 1. 克隆项目
git clone <repository-url>
cd distributed-chat-system

# 2. 编译所有组件
./build.sh

# 或手动编译各个组件
cd server && make
cd ../client && make
cd ../storage && make

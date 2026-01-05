#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <cstring>

// 数据包类型
enum PacketType {
    MSG_TYPE_CHAT = 1,       // 聊天消息
    MSG_TYPE_SYSTEM = 2,     // 系统消息
    MSG_TYPE_SET_NICKNAME = 3, // 设置昵称
    MSG_TYPE_COMMAND = 4     // 命令
};

// 数据包头
struct PacketHeader {
    uint32_t length;         // 数据长度
    uint8_t type;            // 数据包类型
    uint8_t reserved[3];     // 保留字段
};

// 完整数据包
struct Packet {
    PacketHeader header;
    static const size_t MAX_DATA_SIZE = 4096 - sizeof(PacketHeader);
    char data[MAX_DATA_SIZE];
    size_t length;           // 实际数据长度
    
    Packet() {
        memset(&header, 0, sizeof(header));
        memset(data, 0, MAX_DATA_SIZE);
        length = 0;
    }
};

// 存储操作类型
enum StorageOperation {
    STORE_FILE = 1,
    RETRIEVE_FILE = 2,
    DELETE_FILE = 3,
    LIST_FILES = 4
};

// 存储请求
struct StorageRequest {
    StorageOperation op;
    char filename[256];
    size_t data_size;
    uint8_t reserved[4];
};

// 存储响应
struct StorageResponse {
    bool success;
    char message[256];
    size_t data_size;
};

#endif // PROTOCOL_HPP
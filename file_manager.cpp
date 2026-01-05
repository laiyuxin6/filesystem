#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include "lru_cache.hpp"

struct FileMetadata {
    std::string filename;
    size_t size;
    time_t create_time;
    time_t modify_time;
    std::string storage_path;
    std::vector<std::string> replicas; // 副本位置
};

class FileManager {
public:
    FileManager(const std::string& storage_root, size_t cache_capacity = 1000);
    
    // 文件操作
    bool store_file(const std::string& filename, const std::vector<char>& data);
    bool retrieve_file(const std::string& filename, std::vector<char>& data);
    bool delete_file(const std::string& filename);
    bool file_exists(const std::string& filename);
    
    // 文件信息
    FileMetadata get_file_metadata(const std::string& filename);
    std::vector<std::string> list_files();
    
    // 副本管理
    bool add_replica(const std::string& filename, const std::string& replica_node);
    bool remove_replica(const std::string& filename, const std::string& replica_node);
    
    // 统计信息
    size_t get_total_files() const;
    size_t get_total_size() const;
    
private:
    std::string storage_root_;
    std::mutex metadata_mutex_;
    std::unordered_map<std::string, FileMetadata> file_metadata_;
    
    // LRU缓存最近访问的文件
    LRUCache<std::string, std::vector<char>> file_cache_;
    
    // 工具函数
    std::string get_file_path(const std::string& filename);
    void load_metadata();
    void save_metadata();
    void update_metadata(const std::string& filename, const FileMetadata& metadata);
};

#endif // FILE_MANAGER_HPP
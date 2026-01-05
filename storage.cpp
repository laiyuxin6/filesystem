#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "file_manager.hpp"
#include "common/protocol.hpp"

class StorageServer {
public:
    StorageServer(int port, const std::string& storage_path, 
                  const std::vector<std::string>& peers)
        : port_(port), file_manager_(storage_path), peers_(peers) {}
    
    bool init() {
        // 创建socket
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd_ < 0) {
            perror("socket creation failed");
            return false;
        }
        
        // 设置SO_REUSEADDR
        int opt = 1;
        if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt failed");
            return false;
        }
        
        // 绑定地址
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);
        
        if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind failed");
            return false;
        }
        
        // 开始监听
        if (listen(server_fd_, 128) < 0) {
            perror("listen failed");
            return false;
        }
        
        std::cout << "Storage server started on port " << port_ << std::endl;
        std::cout << "Storage path: " << file_manager_.get_total_files() 
                  << " files, " << file_manager_.get_total_size() 
                  << " bytes" << std::endl;
        
        return true;
    }
    
    void start() {
        running_ = true;
        
        // 启动工作线程池
        start_worker_threads(4);
        
        // 主线程接受连接
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) {
                if (running_) {
                    perror("accept failed");
                }
                continue;
            }
            
            // 将连接加入队列
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                connection_queue_.push(client_fd);
                queue_cv_.notify_one();
            }
        }
    }
    
    void stop() {
        running_ = false;
        close(server_fd_);
        queue_cv_.notify_all();
        
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        std::cout << "Storage server stopped." << std::endl;
    }
    
private:
    void start_worker_threads(int num_threads) {
        for (int i = 0; i < num_threads; ++i) {
            worker_threads_.emplace_back([this]() {
                worker_thread();
            });
        }
    }
    
    void worker_thread() {
        while (running_) {
            int client_fd = -1;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this]() {
                    return !connection_queue_.empty() || !running_;
                });
                
                if (!running_ && connection_queue_.empty()) {
                    break;
                }
                
                if (!connection_queue_.empty()) {
                    client_fd = connection_queue_.front();
                    connection_queue_.pop();
                }
            }
            
            if (client_fd >= 0) {
                handle_client(client_fd);
            }
        }
    }
    
    void handle_client(int client_fd) {
        // 接收请求
        StorageRequest request;
        ssize_t bytes_read = recv(client_fd, &request, sizeof(request), 0);
        
        if (bytes_read != sizeof(request)) {
            close(client_fd);
            return;
        }
        
        // 处理请求
        StorageResponse response;
        memset(&response, 0, sizeof(response));
        
        switch (request.op) {
            case STORE_FILE:
                handle_store_request(client_fd, request, response);
                break;
                
            case RETRIEVE_FILE:
                handle_retrieve_request(client_fd, request, response);
                break;
                
            case DELETE_FILE:
                handle_delete_request(client_fd, request, response);
                break;
                
            case LIST_FILES:
                handle_list_request(client_fd, request, response);
                break;
                
            default:
                response.success = false;
                snprintf(response.message, sizeof(response.message), 
                        "Invalid operation");
                send_response(client_fd, response);
                break;
        }
        
        close(client_fd);
    }
    
    void handle_store_request(int client_fd, const StorageRequest& request, 
                             StorageResponse& response) {
        // 接收文件数据
        std::vector<char> data(request.data_size);
        ssize_t total_received = 0;
        
        while (total_received < request.data_size) {
            ssize_t received = recv(client_fd, data.data() + total_received, 
                                   request.data_size - total_received, 0);
            if (received <= 0) {
                response.success = false;
                snprintf(response.message, sizeof(response.message), 
                        "Failed to receive file data");
                send_response(client_fd, response);
                return;
            }
            total_received += received;
        }
        
        // 存储文件
        if (file_manager_.store_file(request.filename, data)) {
            response.success = true;
            response.data_size = data.size();
            snprintf(response.message, sizeof(response.message), 
                    "File stored successfully");
            
            // 创建副本
            create_replicas(request.filename, data);
        } else {
            response.success = false;
            snprintf(response.message, sizeof(response.message), 
                    "Failed to store file");
        }
        
        send_response(client_fd, response);
    }
    
    void handle_retrieve_request(int client_fd, const StorageRequest& request,
                               StorageResponse& response) {
        std::vector<char> data;
        
        if (file_manager_.retrieve_file(request.filename, data)) {
            response.success = true;
            response.data_size = data.size();
            snprintf(response.message, sizeof(response.message), 
                    "File retrieved successfully");
            
            // 发送响应头
            send_response(client_fd, response);
            
            // 发送文件数据
            send(client_fd, data.data(), data.size(), 0);
        } else {
            response.success = false;
            snprintf(response.message, sizeof(response.message), 
                    "File not found");
            send_response(client_fd, response);
        }
    }
    
    void handle_delete_request(int client_fd, const StorageRequest& request,
                             StorageResponse& response) {
        if (file_manager_.delete_file(request.filename)) {
            response.success = true;
            snprintf(response.message, sizeof(response.message), 
                    "File deleted successfully");
            
            // 删除副本
            delete_replicas(request.filename);
        } else {
            response.success = false;
            snprintf(response.message, sizeof(response.message), 
                    "Failed to delete file");
        }
        
        send_response(client_fd, response);
    }
    
    void handle_list_request(int client_fd, const StorageRequest& request,
                           StorageResponse& response) {
        auto files = file_manager_.list_files();
        
        // 构建文件列表字符串
        std::string file_list;
        for (const auto& filename : files) {
            auto metadata = file_manager_.get_file_metadata(filename);
            file_list += filename + " (" + std::to_string(metadata.size) + " bytes)\n";
        }
        
        response.success = true;
        response.data_size = file_list.size();
        snprintf(response.message, sizeof(response.message), 
                "%lu files found", files.size());
        
        // 发送响应头
        send_response(client_fd, response);
        
        // 发送文件列表
        send(client_fd, file_list.c_str(), file_list.size(), 0);
    }
    
    void create_replicas(const std::string& filename, const std::vector<char>& data) {
        for (const auto& peer : peers_) {
            // 在实际系统中，这里会连接到其他节点并发送副本
            // 简化实现：记录副本位置
            file_manager_.add_replica(filename, peer);
        }
    }
    
    void delete_replicas(const std::string& filename) {
        for (const auto& peer : peers_) {
            file_manager_.remove_replica(filename, peer);
        }
    }
    
    void send_response(int client_fd, const StorageResponse& response) {
        send(client_fd, &response, sizeof(response), 0);
    }
    
private:
    int port_;
    int server_fd_;
    std::atomic<bool> running_{false};
    
    FileManager file_manager_;
    std::vector<std::string> peers_;
    
    // 线程池
    std::vector<std::thread> worker_threads_;
    std::queue<int> connection_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <storage_path> [peer1 peer2 ...]" 
                  << std::endl;
        return 1;
    }
    
    int port = atoi(argv[1]);
    std::string storage_path = argv[2];
    
    std::vector<std::string> peers;
    for (int i = 3; i < argc; ++i) {
        peers.push_back(argv[i]);
    }
    
    StorageServer server(port, storage_path, peers);
    
    if (!server.init()) {
        return 1;
    }
    
    server.start();
    
    return 0;
}
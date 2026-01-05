#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace utils {
    
    // 获取当前时间戳
    inline std::string get_current_time() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    // 分割字符串
    inline std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream token_stream(str);
        
        while (std::getline(token_stream, token, delimiter)) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    // 去除字符串两端空格
    inline std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, last - first + 1);
    }
    
    // 生成随机字符串
    inline std::string generate_random_string(size_t length) {
        static const char charset[] = 
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        
        std::string result;
        result.reserve(length);
        
        for (size_t i = 0; i < length; ++i) {
            result.push_back(charset[rand() % (sizeof(charset) - 1)]);
        }
        
        return result;
    }
    
    // 格式化字节大小
    inline std::string format_bytes(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        double size = bytes;
        int unit_idx = 0;
        
        while (size >= 1024 && unit_idx < 4) {
            size /= 1024;
            unit_idx++;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << units[unit_idx];
        return ss.str();
    }
    
} // namespace utils

#endif // UTILS_HPP
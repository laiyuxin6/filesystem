#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <unordered_map>
#include <list>
#include <mutex>
#include <string>
#include <vector>

template<typename Key, typename Value>
class LRUCache {
public:
    LRUCache(size_t capacity) : capacity_(capacity) {}
    
    // 获取值，如果存在则移动到最前面
    bool get(const Key& key, Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return false;
        }
        
        // 移动到链表头部
        cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
        value = it->second->second;
        return true;
    }
    
    // 插入或更新值
    void put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            // 更新现有值并移动到头部
            it->second->second = value;
            cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
            return;
        }
        
        // 插入新值
        if (cache_map_.size() >= capacity_) {
            // 移除最久未使用的
            auto last = cache_list_.end();
            last--;
            cache_map_.erase(last->first);
            cache_list_.pop_back();
        }
        
        cache_list_.push_front({key, value});
        cache_map_[key] = cache_list_.begin();
    }
    
    // 删除键值对
    void erase(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            cache_list_.erase(it->second);
            cache_map_.erase(it);
        }
    }
    
    // 清空缓存
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_list_.clear();
        cache_map_.clear();
    }
    
    // 获取当前大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_map_.size();
    }
    
    // 获取所有键
    std::vector<Key> keys() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Key> keys;
        keys.reserve(cache_map_.size());
        
        for (const auto& pair : cache_map_) {
            keys.push_back(pair.first);
        }
        
        return keys;
    }
    
private:
    size_t capacity_;
    mutable std::mutex mutex_;
    
    // 链表存储访问顺序，map用于快速查找
    std::list<std::pair<Key, Value>> cache_list_;
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> cache_map_;
};

#endif // LRU_CACHE_HPP
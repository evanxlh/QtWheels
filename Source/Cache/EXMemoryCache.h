//
//  EXMemoryCache.h
//
//  Created by evanxlh on 2025/6/15.
//

#pragma once

#include <list>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <chrono>
#include <memory>
#include <utility>

/**
高效的 LRU 内存缓存: LRU（Least Recently Used）是一种常见的缓存淘汰策略。
当缓存满了，需要为新的数据项腾出空间时，LRU策略会选择最近最少使用的数据项进行淘汰。

 1. 使用std::list来存储缓存项的顺序，因为会频繁地在链表的头部和尾部插入和删除元素。
 2. 使用std::unordered_map来存储键到链表迭代器的映射，可以在O(1)时间内找到任何给定键的元素，避免了链表的O(n)访问时间。

 @note Value 支持`值类型`与`智能指针`类型.
 */
template <typename Key, typename Value>
class EXMemoryCache
{
public:
    struct Config
    {
        // 缓存最大占用内存字节数(0: 无限制)
        size_t costLimit = 0;

        // 最大缓存项数量(0: 无限制)
        size_t countLimit = 0;

        // 默认 TTL 为 3600 秒: 存活时间到了，缓存项会被标记为已过期，然后从缓存中清除。
        // 如果 `enablesTTL = true`, 又没有指定有效的 TTL, 就会用到 `defaultTTL`
        size_t defaultTTL{ 3600 };

        // 是否开启缓存项的存活时间(Time To Live): 默认不开启
        bool enablesTTL = false;

        // 是否开启线程安全: 默认开启
        bool enablesThreadSafe = true;
    };

    explicit EXMemoryCache(Config config = {}) : m_config(config) {}
    ~EXMemoryCache() { clear(); }

    std::optional<Value> get(const Key& key)
    {
        if (m_config.enablesThreadSafe) {
            std::unique_lock lock(m_mutex);
            return _get(key);
        }

        return _get(key);
    }

    // 插入缓存项（完美转发）
    template <typename K, typename V>
    void put(K&& key, V&& value, size_t cost = 1, int ttl = 0)
    {
        auto newItem = std::make_shared<CacheItem>();
        newItem->key = std::forward<K>(key);
        newItem->value = std::forward<V>(value);
        newItem->cost = cost;

        if (m_config.enablesTTL) {
            const auto now = Clock::now();
            if (ttl > 0) {
                newItem->expiration = now + std::chrono::seconds(ttl);
            } else if (m_config.defaultTTL > 0) {
                newItem->expiration = now + std::chrono::seconds(m_config.defaultTTL);
            }
        }

        if (m_config.enablesThreadSafe) {
            std::unique_lock lock(m_mutex);
            _put(std::move(newItem));
        } else {
            _put(std::move(newItem));
        }
    }

    bool remove(const Key& key)
    {
        if (m_config.enablesThreadSafe) {
            std::unique_lock lock(m_mutex);
            return _remove(key);
        }

        return _remove(key);
    }

    // 清空缓存
    void clear()
    {
        if (m_config.enablesThreadSafe) {
            std::unique_lock lock(m_mutex);
            _clear();
        } else {
            _clear();
        }
    }

    // 缓存占用的内存字节数
    size_t totalCost() const
    {
        if (m_config.enablesThreadSafe) {
            std::unique_lock lock(m_mutex);
            return m_totalCost;
        }
        return m_totalCost;
    }

    // 缓存项数量
    size_t count() const
    {
        if (m_config.enablesThreadSafe) {
            std::unique_lock lock(m_mutex);
            return m_cacheMap.size();
        }
        return m_cacheMap.size();
    }

private:

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct CacheItem {
        Key key;
        Value value;
        size_t cost;
        TimePoint lastAccess;
        TimePoint expiration;
    };

    using ItemPtr = std::shared_ptr<CacheItem>;
    using ItemList = std::list<ItemPtr>;
    using CacheMap = std::unordered_map<Key, typename ItemList::iterator>;

    inline bool _isItemExpired(const ItemPtr& item, const TimePoint& now) const
    {
        return item->expiration.time_since_epoch().count() > 0 && now >= item->expiration;
    }

    inline bool _shouldTrim() const
    {
        if (m_config.costLimit > 0 && m_totalCost > m_config.costLimit) return true;
        if (m_config.countLimit > 0 && m_cacheMap.size() > m_config.countLimit) return true;
        return false;
    }

    void _trim()
    {
        while (!m_itemList.empty() && _shouldTrim()) {
            auto& item = m_itemList.front();
            m_totalCost -= item->cost;
            m_cacheMap.erase(item->key);
            m_itemList.pop_front();
        }
    }

    std::optional<Value> _get(const Key& key)
    {
        auto it = m_cacheMap.find(key);
        if (it == m_cacheMap.end()) return std::nullopt;

        const auto now = Clock::now();
        auto& item = *it->second;
        if (m_config.enablesTTL && _isItemExpired(item, now)) {
            _remove(key);
            return std::nullopt;
        }

        // 更新访问时间并移至LRU尾部
        item->lastAccess = now;
        m_itemList.splice(m_itemList.end(), m_itemList, it->second);

        return item->value;
    }

    void _put(ItemPtr newItem)
    {
        if (auto it = m_cacheMap.find(newItem->key); it != m_cacheMap.end()) {
            _remove(newItem->key);
        }

        // 插入新项到LRU尾部
        auto listIt = m_itemList.insert(m_itemList.end(), std::move(newItem));
        auto item = *listIt;
        m_cacheMap.emplace(item->key, listIt);
        m_totalCost += item->cost;

        _trim();
    }

    bool _remove(const Key& key)
    {
        if (auto it = m_cacheMap.find(key); it != m_cacheMap.end()) {
            m_totalCost -= (*it->second)->cost;
            m_itemList.erase(it->second);
            m_cacheMap.erase(it);
            return true;
        }
        return false;
    }

    void _clear() {
        m_itemList.clear();
        m_cacheMap.clear();
        m_totalCost = 0;
    }

private:
    Config m_config;
    ItemList m_itemList;  // LRU队列（头部最旧，尾部最新）
    CacheMap m_cacheMap;  // 快速查找表
    size_t m_totalCost = 0;
    mutable std::mutex m_mutex;
};

module;

#include <cassert>

export module mo_yanxi.cache.map;

import std;

namespace mo_yanxi{
// 使用 C++20 Concepts 探测容器是否具备 reserve 方法
// 从而兼容 std::map 这种没有 reserve 方法的替代容器
template <typename T>
concept has_reserve_method = requires(T t, std::size_t n) {
    t.reserve(n);
};

template <typename KeyType, typename ValueType>
struct cache_node {
	using key_type = KeyType;
	using value_type = ValueType;

	key_type key;
	value_type value;
	std::uint32_t prev;
	std::uint32_t next;
};

export
template <
    typename KeyType,
    typename ValueType,
    template <typename...> class MapTemplate = std::unordered_map,
    template <typename...> class NodeTemplate = std::vector
>
class mapped_lru_cache {
private:
	using cache_node_type = cache_node<KeyType, ValueType>;
    using key_type = cache_node_type::key_type;
	using value_type = cache_node_type::value_type;

    // 实例化具体的容器类型
    using map_type = MapTemplate<key_type, std::uint32_t>;
    using node_type = NodeTemplate<cache_node_type>;

    std::uint32_t capacity_;
    std::uint32_t free_head_;
    node_type nodes_;
    map_type map_;

    // 内部方法：使用 this-> 限定名防止 ADL (Argument-Dependent Lookup) 意外查找到外部函数
    void move_to_front(std::uint32_t index) {
        nodes_[nodes_[index].prev].next = nodes_[index].next;
        nodes_[nodes_[index].next].prev = nodes_[index].prev;
        
        nodes_[index].next = nodes_[0].next;
        nodes_[index].prev = 0;
        nodes_[nodes_[0].next].prev = index;
        nodes_[0].next = index;
    }

public:
	[[nodiscard]] mapped_lru_cache() = default;

	explicit mapped_lru_cache(std::uint32_t capacity)
        : capacity_(capacity), free_head_(1), nodes_(capacity + 1) {
        assert(capacity > 0 && "Cache capacity must be greater than 0");
        
        // 编译期条件分支：如果用户提供的 Map 容器有 reserve 接口才调用
        if constexpr (has_reserve_method<map_type>) {
            map_.reserve(capacity);
        }
        
        nodes_[0].prev = 0;
        nodes_[0].next = 0;
        
        for (std::uint32_t i = 1; i < capacity; ++i) {
            nodes_[i].next = i + 1;
        }
        nodes_[capacity].next = 0; 
    }

	std::size_t capacity() const noexcept{
		return capacity_;
	}

    [[nodiscard]] std::optional<value_type> get(const key_type& key) noexcept {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }
        
        std::uint32_t index = it->second;
        this->move_to_front(index); 
        return nodes_[index].value;
    }

    [[nodiscard]] value_type* get_ptr(const key_type& key) noexcept {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return nullptr;
        }

        std::uint32_t index = it->second;
        this->move_to_front(index);
        return std::addressof(nodes_[index].value);
    }

    void put(const key_type& key, const value_type& value) {
        auto it = map_.find(key);
        
        if (it != map_.end()) {
            std::uint32_t index = it->second;
            nodes_[index].value = value;
            this->move_to_front(index);
            return;
        }

        std::uint32_t new_idx;
        
        if (free_head_ != 0) {
            new_idx = free_head_;
            free_head_ = nodes_[free_head_].next;
            
            nodes_[new_idx].next = nodes_[0].next;
            nodes_[new_idx].prev = 0;
            nodes_[nodes_[0].next].prev = new_idx;
            nodes_[0].next = new_idx;
        } else {
            new_idx = nodes_[0].prev; 
            map_.erase(nodes_[new_idx].key);
            this->move_to_front(new_idx);
        }

        nodes_[new_idx].key = key;
        nodes_[new_idx].value = value;
        map_[key] = new_idx;
    }
};
}

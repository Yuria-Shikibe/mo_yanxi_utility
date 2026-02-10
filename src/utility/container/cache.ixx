export module mo_yanxi.cache;

import mo_yanxi.meta_programming;
import std;

namespace mo_yanxi {
    struct nit_{};
    /**
     * @brief Union 槽位，避免默认构造，且支持 constexpr 访问
     */
    template <typename T>
    union slot_t {
        T value;
        nit_ uninit_{};

        constexpr slot_t() noexcept {}
        constexpr ~slot_t() noexcept {} // 具体的析构由 lru_cache 控制
    };

    /**
     * @brief 基于 SoA 布局与 Union 存储的 Constexpr LRU 缓存
     */
    export
    template <typename K, typename V, std::size_t N>
        requires std::equality_comparable<K> && (N > 0)
    class lru_cache {
        using size_type = smallest_uint_t<N>;
        using key_type = K;
        using value_type = V;

    private:
        static constexpr size_type invalid_index = std::numeric_limits<size_type>::max();

        struct link_node {
            size_type prev = invalid_index;
            size_type next = invalid_index;
        };


        // --- 存储区 (SoA Layout) ---
        // 使用 union 数组替代 byte 数组，天然对齐且允许 constexpr 访问
        std::array<slot_t<K>, N> keys_storage_;
        std::array<slot_t<V>, N> values_storage_;

        // 链表节点数组
        std::array<link_node, N> links_;

        size_type head_ = invalid_index;
        size_type tail_ = invalid_index;
        size_type free_head_ = 0;
        size_type size_ = 0;

        // 注意：std::bitset 的 constexpr 修改操作 (set/reset) 需要 C++23。
        // 在 C++20 环境下，如果编译器不支持 constexpr bitset，可能需要用 bool 数组或整数位掩码代替。
        std::bitset<N> active_mask_;

    public:
        constexpr lru_cache() {
            for (size_type i = 0; i < N; ++i) {
                this->links_[i].next = (i + 1 < N) ?
                    i + 1 : invalid_index;
            }
        }

        constexpr ~lru_cache() requires(!std::is_trivially_destructible_v<K> || !std::is_trivially_destructible_v<V>) {
            clear_resources();
        }

        constexpr ~lru_cache() requires(!(!std::is_trivially_destructible_v<K> || !std::is_trivially_destructible_v<V>)) = default;

        constexpr lru_cache(const lru_cache& other) noexcept(
            std::is_nothrow_copy_constructible_v<K> &&
            std::is_nothrow_copy_constructible_v<V>
        ) {
            this->copy_from(other);
        }

        constexpr lru_cache(lru_cache&& other) noexcept(
            std::is_nothrow_move_constructible_v<K> &&
            std::is_nothrow_move_constructible_v<V>
        ) {
            head_ = other.head_;
            tail_ = other.tail_;
            free_head_ = other.free_head_;
            size_ = other.size_;
            links_ = other.links_;

            for (size_type i = 0; i < N; ++i) {
                if (other.active_mask_.test(i)) {
                    // 使用 union 成员地址进行构造
                    std::construct_at(get_raw_key_ptr(i), std::move(*other.get_key_ptr(i)));
                    std::construct_at(get_raw_val_ptr(i), std::move(*other.get_val_ptr(i)));
                    active_mask_.set(i);
                }
            }

            other.active_mask_.reset();
            other.size_ = 0;
            other.head_ = invalid_index;
            other.tail_ = invalid_index;
            other.free_head_ = invalid_index;
        }

        constexpr lru_cache& operator=(const lru_cache& other) noexcept(
            std::is_nothrow_copy_constructible_v<K> &&
            std::is_nothrow_copy_constructible_v<V>
        ) {
            if (this != &other) {
                this->clear_resources();
                this->copy_from(other);
            }
            return *this;
        }

        constexpr lru_cache& operator=(lru_cache&& other) noexcept(
            std::is_nothrow_move_constructible_v<K> &&
            std::is_nothrow_move_constructible_v<V>
        ) {
            if (this != &other) {
                this->clear_resources();

                head_ = other.head_;
                tail_ = other.tail_;
                free_head_ = other.free_head_;
                size_ = other.size_;
                links_ = other.links_;

                for (size_type i = 0; i < N; ++i) {
                    if (other.active_mask_.test(i)) {
                        std::construct_at(this->get_raw_key_ptr(i), std::move(*other.get_key_ptr(i)));
                        std::construct_at(this->get_raw_val_ptr(i), std::move(*other.get_val_ptr(i)));
                        active_mask_.set(i);
                    }
                }

                other.active_mask_.reset();
                other.size_ = 0;
                other.head_ = invalid_index;
                other.tail_ = invalid_index;
                other.free_head_ = invalid_index;
            }
            return *this;
        }

    private:
        constexpr void copy_from(const lru_cache& other) {
            head_ = other.head_;
            tail_ = other.tail_;
            free_head_ = other.free_head_;
            size_ = other.size_;
            links_ = other.links_;

            for (size_type i = 0; i < N; ++i) {
                if (other.active_mask_.test(i)) {
                    std::construct_at(get_raw_key_ptr(i), *other.get_key_ptr(i));
                    try {
                        std::construct_at(get_raw_val_ptr(i), *other.get_val_ptr(i));
                    } catch (...) {
                        std::destroy_at(get_key_ptr(i));
                        throw;
                    }
                    active_mask_.set(i);
                }
            }
        }

        constexpr void clear_resources() noexcept {
            if constexpr (!std::is_trivially_destructible_v<K> || !std::is_trivially_destructible_v<V>) {
                for (size_type i = 0; i < N; ++i) {
                    if (active_mask_.test(i)) {
                        std::destroy_at(this->get_key_ptr(i));
                        std::destroy_at(this->get_val_ptr(i));
                    }
                }
            }
            active_mask_.reset();
            size_ = 0;
        }

    public:
        constexpr void clear() noexcept {
            clear_resources();
            head_ = invalid_index;
            tail_ = invalid_index;
            free_head_ = 0;
            for (size_type i = 0; i < N; ++i) {
                this->links_[i].next = (i + 1 < N) ? i + 1 : invalid_index;
                this->links_[i].prev = invalid_index; // 确保清理彻底
            }
        }

        template <typename... Args>
            requires (std::constructible_from<value_type, Args&&...>)
        constexpr void emplace(K&& key, Args&&... args) {
            if (auto idx = this->find_index(key); idx != invalid_index) {
                std::destroy_at(this->get_val_ptr(idx));
                std::construct_at(this->get_raw_val_ptr(idx), std::forward<Args>(args)...);
                this->move_to_head(idx);
                return;
            }

            size_type target_idx = invalid_index;
            bool is_eviction = false;

            if (this->size_ < N) {
                target_idx = this->allocate_node();
            } else {
                target_idx = this->tail_;
                is_eviction = true;
                this->remove_node(this->tail_);
            }

            if (is_eviction) {
                std::destroy_at(this->get_key_ptr(target_idx));
                std::destroy_at(this->get_val_ptr(target_idx));
            }

            std::construct_at(this->get_raw_key_ptr(target_idx), std::move(key));
            std::construct_at(this->get_raw_val_ptr(target_idx), std::forward<Args>(args)...);

            this->active_mask_.set(target_idx);
            this->push_front(target_idx);
        }

        template <typename... Args>
            requires (std::constructible_from<value_type, Args&&...>)
        constexpr void emplace(const K& key, Args&&... args) {
            this->emplace(K{key}, std::forward<Args&&>(args)...);
        }

        constexpr void put(K&& key, V&& value) {
            this->emplace(std::move(key), std::move(value));
        }

        constexpr void put(const K& key, V&& value) {
            this->emplace(key_type{key}, std::move(value));
        }

        constexpr void put(const K& key, const V& value) {
            this->emplace(key_type{key}, value_type{value});
        }

        constexpr V* get(const K& key) {
            auto idx = this->find_index(key);
            if (idx == invalid_index) {
                return nullptr;
            }
            this->move_to_head(idx);
            return this->get_val_ptr(idx);
        }

        [[nodiscard]] constexpr bool contains(const K& key) const {
            size_type curr = this->head_;
            while (curr != invalid_index) {
                if (*this->get_key_ptr(curr) == key) {
                    return true;
                }
                curr = this->links_[curr].next;
            }
            return false;
        }

        [[nodiscard]] constexpr size_type size() const noexcept { return this->size_; }
        [[nodiscard]] constexpr size_type capacity() const noexcept { return N; }
        [[nodiscard]] constexpr bool empty() const noexcept { return this->size_ == 0; }

    private:
        // --- 核心修改：通过 union 成员访问，去除 reinterpret_cast ---

        constexpr K* get_raw_key_ptr(size_type idx) {
            return &keys_storage_[idx].value;
        }

        constexpr V* get_raw_val_ptr(size_type idx) {
            return &values_storage_[idx].value;
        }

        // 使用 std::launder 保持标准兼容性，同时在 constexpr 中有效
        constexpr K* get_key_ptr(size_type idx) {
            return std::launder(&keys_storage_[idx].value);
        }

        constexpr const K* get_key_ptr(size_type idx) const {
            return std::launder(&keys_storage_[idx].value);
        }

        constexpr V* get_val_ptr(size_type idx) {
            return std::launder(&values_storage_[idx].value);
        }

        constexpr const V* get_val_ptr(size_type idx) const {
            return std::launder(&values_storage_[idx].value);
        }

        constexpr size_type find_index(const K& key) const {
            size_type curr = this->head_;
            while (curr != invalid_index) {
                if (*this->get_key_ptr(curr) == key) {
                    return curr;
                }
                curr = this->links_[curr].next;
            }
            return invalid_index;
        }

        constexpr size_type allocate_node() {
            if (this->free_head_ == invalid_index) return invalid_index;
            size_type idx = this->free_head_;
            this->free_head_ = this->links_[idx].next;
            this->size_++;
            return idx;
        }

        constexpr void move_to_head(size_type idx) {
            if (idx == this->head_) return;
            this->remove_node(idx);
            this->push_front(idx);
        }

        constexpr void remove_node(size_type idx) {
            auto& link = this->links_[idx];
            if (link.prev != invalid_index) {
                this->links_[link.prev].next = link.next;
            } else {
                this->head_ = link.next;
            }

            if (link.next != invalid_index) {
                this->links_[link.next].prev = link.prev;
            } else {
                this->tail_ = link.prev;
            }
        }

        constexpr void push_front(size_type idx) {
            this->links_[idx].next = this->head_;
            this->links_[idx].prev = invalid_index;

            if (this->head_ != invalid_index) {
                this->links_[this->head_].prev = idx;
            }

            this->head_ = idx;
            if (this->tail_ == invalid_index) {
                this->tail_ = idx;
            }
        }
    };
}
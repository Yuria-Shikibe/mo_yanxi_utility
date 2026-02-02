//
// Created by Matrix on 2026/1/30.
//

export module mo_yanxi.cache;

import std;

namespace mo_yanxi {
    template <unsigned long long N>
    struct get_smallest_uint{
        using type =
        std::conditional_t<(N <= 0xFF), std::uint8_t,
            std::conditional_t<(N <= 0xFFFF), std::uint16_t,
                std::conditional_t<(N <= 0xFFFFFFFF), std::uint32_t,
                    std::uint64_t>>>;
    };

    template <unsigned long long N>
    using smallest_uint_t = typename get_smallest_uint<N>::type;

    /**
     * @brief 基于 SoA 布局与原始内存管理的 LRU 缓存
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

        // --- 原始内存存储区 ---
        alignas(K) std::array<std::byte, N * sizeof(K)> keys_buf_;
        alignas(V) std::array<std::byte, N * sizeof(V)> values_buf_;

        alignas(std::hardware_constructive_interference_size) std::array<link_node, N> links_;

        size_type head_ = invalid_index;
        size_type tail_ = invalid_index;
        size_type free_head_ = 0;
        size_type size_ = 0;

        std::bitset<N> active_mask_;

    public:
        lru_cache() {
            for (size_type i = 0; i < N; ++i) {
                this->links_[i].next = (i + 1 < N) ? i + 1 : invalid_index;
            }
        }

        ~lru_cache() {
            if constexpr (!std::is_trivially_destructible_v<K> || !std::is_trivially_destructible_v<V>) {
                for (size_type i = 0; i < N; ++i) {
                    if (active_mask_.test(i)) {
                        std::destroy_at(this->get_key_ptr(i));
                        std::destroy_at(this->get_val_ptr(i));
                    }
                }
            }
        }

        lru_cache(const lru_cache& other) {
            copy_from(other);
        }

        /**
         * @brief 移动构造函数
         * 从 other 移动构造所有元素，并接管链表结构。
         * 注意：由于是栈上内存布局，这里不是指针交换，而是逐个元素的 Move Construct。
         */
        lru_cache(lru_cache&& other) noexcept(
            std::is_nothrow_move_constructible_v<K> &&
            std::is_nothrow_move_constructible_v<V>
        ) {
            // 1. 复制元数据
            head_ = other.head_;
            tail_ = other.tail_;
            free_head_ = other.free_head_;
            size_ = other.size_;
            links_ = other.links_;

            // 2. 移动构造有效元素
            for (size_type i = 0; i < N; ++i) {
                if (other.active_mask_.test(i)) {
                    std::construct_at(get_raw_key_ptr(i), std::move(*other.get_key_ptr(i)));
                    std::construct_at(get_raw_val_ptr(i), std::move(*other.get_val_ptr(i)));
                    active_mask_.set(i);
                }
            }

            // 3. 重置源对象 (防止 Double Free)
            // 即使 other 的元素已被 move，other 的析构函数仍会运行。
            // 必须清空 other 的 active_mask_，使其析构函数“无事可做”。
            other.active_mask_.reset();
            other.size_ = 0;
            other.head_ = invalid_index;
            other.tail_ = invalid_index;
            other.free_head_ = invalid_index; // 或重置为初始链表，但设为 invalid 足以保证安全
        }

        /**
         * @brief 复制赋值运算符
         * 采用 "Destroy then Copy" 策略
         */
        lru_cache& operator=(const lru_cache& other) {
            if (this != &other) {
                // 1. 清理当前对象资源
                this->clear_resources();

                // 2. 调用复制逻辑
                this->copy_from(other);
            }
            return *this;
        }

        /**
         * @brief 移动赋值运算符
         */
        lru_cache& operator=(lru_cache&& other) noexcept(
            std::is_nothrow_move_constructible_v<K> &&
            std::is_nothrow_move_constructible_v<V>
        ) {
            if (this != &other) {
                // 1. 清理当前对象
                this->clear_resources();

                // 2. 移动数据
                head_ = other.head_;
                tail_ = other.tail_;
                free_head_ = other.free_head_;
                size_ = other.size_;
                links_ = other.links_;

                for (size_type i = 0; i < N; ++i) {
                    if (other.active_mask_.test(i)) {
                        std::construct_at(get_raw_key_ptr(i), std::move(*other.get_key_ptr(i)));
                        std::construct_at(get_raw_val_ptr(i), std::move(*other.get_val_ptr(i)));
                        active_mask_.set(i);
                    }
                }

                // 3. 重置源对象
                other.active_mask_.reset();
                other.size_ = 0;
                other.head_ = invalid_index;
                other.tail_ = invalid_index;
                other.free_head_ = invalid_index;
            }
            return *this;
        }

    private:
        void copy_from(const lru_cache& other) {
            // 1. 复制元数据
            head_ = other.head_;
            tail_ = other.tail_;
            free_head_ = other.free_head_;
            size_ = other.size_;
            links_ = other.links_; // link_node 是 trivial 的，可以直接复制

            // 2. 深度复制有效元素 (基于 active_mask_)
            for (size_type i = 0; i < N; ++i) {
                if (other.active_mask_.test(i)) {
                    // 构造 Key
                    std::construct_at(get_raw_key_ptr(i), *other.get_key_ptr(i));

                    try {
                        // 构造 Value
                        std::construct_at(get_raw_val_ptr(i), *other.get_val_ptr(i));
                    } catch (...) {
                        // 强异常保证：如果 Value 构造失败，需销毁已构造的 Key
                        std::destroy_at(get_key_ptr(i));
                        // 此时 active_mask_ 尚未设置该位，析构函数不会重复销毁
                        throw;
                    }

                    // 构造成功后标记为活跃
                    active_mask_.set(i);
                }
            }
        }

        // 辅助函数：清理当前所有资源（用于赋值前或析构）
        void clear_resources() noexcept {
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
            // 不需要重置 links_，因为会被立即覆盖
        }

    public:
    	void clear() noexcept{
    		if constexpr (!std::is_trivially_destructible_v<K> || !std::is_trivially_destructible_v<V>){
    			for (size_type i = 0; i < N; ++i) {
    				if (active_mask_.test(i)) {
    					std::destroy_at(this->get_key_ptr(i));
    					std::destroy_at(this->get_val_ptr(i));
    				}
    			}
    		}

    		active_mask_.reset();
    		size_ = 0;
    		head_ = invalid_index;
    		tail_ = invalid_index;
    		free_head_ = 0;

    		for (size_type i = 0; i < N; ++i) {
    			this->links_[i].next = (i + 1 < N) ? i + 1 : invalid_index;
    		}
    	}

        /**
         * @brief 原位构造插入 (核心方法)
         * 直接在缓存内存中构造 Value，避免临时对象和移动开销。
         * 如果 Key 已存在，将销毁旧值并用 args 原位构造新值。
         */
        template <typename... Args>
            requires (std::constructible_from<value_type, Args&&...>)
        void emplace(K&& key, Args&&... args) {
            // 1. 尝试查找
            if (auto idx = this->find_index(key); idx != invalid_index) {
                // 已存在：更新逻辑
                // 策略：原地销毁旧值 -> 原地构造新值
                // 优点：支持不可赋值(operator= deleted)但可构造的类型
                std::destroy_at(this->get_val_ptr(idx));
                std::construct_at(this->get_raw_val_ptr(idx), std::forward<Args>(args)...);

                this->move_to_head(idx);
                return;
            }

            // 2. 准备节点 (分配或驱逐)
            size_type target_idx = invalid_index;
            bool is_eviction = false;

            if (this->size_ < N) {
                target_idx = this->allocate_node();
            } else {
                target_idx = this->tail_;
                is_eviction = true;
                this->remove_node(this->tail_); // 逻辑移除
            }

            // 3. 生命周期管理
            if (is_eviction) {
                // 必须显式析构被驱逐的对象
                std::destroy_at(this->get_key_ptr(target_idx));
                std::destroy_at(this->get_val_ptr(target_idx));
            }

            // 4. 原位构造 (Placement New)
            // 注意：key 在这里才被 move，如果上面 find_index 成功，key 不会被 move
            std::construct_at(this->get_raw_key_ptr(target_idx), std::move(key));
            std::construct_at(this->get_raw_val_ptr(target_idx), std::forward<Args>(args)...);

            this->active_mask_.set(target_idx);

            // 5. 更新链表
            this->push_front(target_idx);
        }

        template <typename... Args>
            requires (std::constructible_from<value_type, Args&&...>)
        void emplace(const K& key, Args&&... args) {
            this->emplace(K{key}, std::forward<Args&&>(args)...);
        }

        // --- Put 包装器 (均转发至 emplace) ---

        void put(K&& key, V&& value) {
            this->emplace(std::move(key), std::move(value));
        }

        void put(const K& key, V&& value) {
            this->emplace(key_type{key}, std::move(value));
        }

        void put(const K& key, const V& value) {
            this->emplace(key_type{key}, value_type{value});
        }


        /**
         * @brief 获取值指针
         */
        V* get(const K& key) {
            auto idx = this->find_index(key);
            if (idx == invalid_index) {
                return nullptr;
            }
            this->move_to_head(idx);
            return get_val_ptr(idx);
        }

        [[nodiscard]] bool contains(const K& key) const {
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
        K* get_raw_key_ptr(size_type idx) {
            return reinterpret_cast<K*>(&keys_buf_[idx * sizeof(K)]);
        }

        V* get_raw_val_ptr(size_type idx) {
            return reinterpret_cast<V*>(&values_buf_[idx * sizeof(V)]);
        }


        //TODO is launder here necessary

        K* get_key_ptr(size_type idx) {
            return std::launder(reinterpret_cast<K*>(&keys_buf_[idx * sizeof(K)]));
        }

        const K* get_key_ptr(size_type idx) const {
            return std::launder(reinterpret_cast<const K*>(&keys_buf_[idx * sizeof(K)]));
        }

        V* get_val_ptr(size_type idx) {
            return std::launder(reinterpret_cast<V*>(&values_buf_[idx * sizeof(V)]));
        }

        const V* get_val_ptr(size_type idx) const {
            return std::launder(reinterpret_cast<const V*>(&values_buf_[idx * sizeof(V)]));
        }

        size_type find_index(const K& key) const {
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
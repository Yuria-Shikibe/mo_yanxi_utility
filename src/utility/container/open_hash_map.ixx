module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.open_addr_hash_map;

import std;

namespace mo_yanxi {
    export struct bad_hash_map_key : std::runtime_error {
        [[nodiscard]] bad_hash_map_key() : std::runtime_error{"bad_hash_map_key"} {}
        using std::runtime_error::runtime_error;
    };

    template <typename T>
    struct arrow_proxy {
    private:
        T inner_pair;
    public:
        [[nodiscard]] explicit arrow_proxy(const T& inner_pair)
            : inner_pair(inner_pair) {}

        [[nodiscard]] explicit arrow_proxy(T&& inner_pair)
            : inner_pair(std::move(inner_pair)) {}

        constexpr const T* operator->() const noexcept {
            return &inner_pair;
        }
    };

    template <typename T>
    concept Transparent = requires{
        typename T::is_transparent;
    };

    // 优化 1: 引入 Hash Mixer，解决 Power-of-2 哈希表的低位冲突问题
    struct hash_mixer {
        [[nodiscard]] static constexpr std::size_t mix(std::size_t x) noexcept {
            // MurmurHash3 fmix64 风格的混淆器
            x ^= x >> 33;
            x *= 0xff51afd7ed558ccdULL;
            x ^= x >> 33;
            x *= 0xc4ceb9fe1a85ec53ULL;
            x ^= x >> 33;
            return x;
        }
    };

    struct pointer_transparent_hasher {
        template <typename T>
            requires (std::is_pointer_v<T>)
        static constexpr std::size_t operator()(T ptr) noexcept {
            // 简单的转换，依赖外部 Mixer 进行混淆
            return reinterpret_cast<std::size_t>(ptr);
        }
        using is_transparent = void;
    };

    export
    template <
        typename Key, typename Val, typename EmptyKey, auto emptyKey,
        typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>,
        std::invocable<const EmptyKey&> Convertor = std::identity,
        typename Allocator = std::allocator<std::pair<Key, Val>>>
    struct fixed_open_addr_hash_map {
        using key_type = Key;
        using mapped_type = Val;
        using hasher = Hash;
        using key_equal = KeyEqual;

        // 优化 2: 定义最大负载因子 (0.75 这里的实现是 3/4)
        static constexpr double MAX_LOAD_FACTOR = 0.75;

    private:
        template <typename K>
        static constexpr bool isHasherValid = (std::same_as<K, key_type> || Transparent<hasher>) && std::is_invocable_r_v<
            std::size_t, hasher, const K&>;
        template <typename K>
        static constexpr bool isEqualerValid = std::same_as<K, key_type> || Transparent<key_equal>;

        static constexpr decltype(auto) getStaticEmptyKey() noexcept {
            return empty_key();
        }

    public:
        using value_type = std::pair<const key_type, mapped_type>;

    private:
        template <typename K>
        FORCE_INLINE
        constexpr static bool is_empty_key(const K& key) noexcept {
            if constexpr(std::predicate<key_equal, const K&> && requires{
                typename key_equal::is_direct;
            }) {
                return KeyEqualer(key);
            } else {
                return KeyEqualer(getStaticEmptyKey(), key);
            }
        }

        struct mapped_buffer {
            static constexpr std::size_t mapped_type_size = sizeof(mapped_type);
            static constexpr std::size_t mapped_type_align = alignof(mapped_type);
            alignas(mapped_type_align) std::array<std::byte, mapped_type_size> buffer;
            [[nodiscard]] constexpr const mapped_type* data() const noexcept {
                return reinterpret_cast<const mapped_type*>(buffer.data());
            }

            [[nodiscard]] constexpr mapped_type* data() noexcept {
                return reinterpret_cast<mapped_type*>(buffer.data());
            }
        };

        struct kv_storage {
            key_type key{};
            ADAPTED_NO_UNIQUE_ADDRESS mapped_buffer value;

            [[nodiscard]] constexpr explicit(false) kv_storage(const key_type key)
                : key(key) {}

            template <typename T>
                requires std::constructible_from<key_type, T>
            [[nodiscard]] constexpr explicit(false) kv_storage(T&& key)
                : key(std::forward<T>(key)) {}

            [[nodiscard]] constexpr kv_storage() : key(getStaticEmptyKey()) {}

            constexpr explicit operator bool() const noexcept {
                return !fixed_open_addr_hash_map::is_empty_key(key);
            }

            constexpr ~kv_storage() {
                try_destroy();
            }

            constexpr void try_destroy() noexcept {
                if constexpr (std::is_trivially_destructible_v<mapped_type>) return;
                if (!fixed_open_addr_hash_map::is_empty_key(key)) {
                    std::destroy_at(value.data());
                }
            }

            constexpr void destroy() noexcept {
                if constexpr (std::is_trivially_destructible_v<mapped_type>) return;
                std::destroy_at(value.data());
            }

            template <typename ...Args>
                requires (std::constructible_from<mapped_type, Args...>)
            constexpr void emplace(Args&& ...args) noexcept(std::is_nothrow_constructible_v<mapped_type, Args...>) {
                std::construct_at(value.data(), std::forward<Args>(args)...);
            }

            // Copy/Move 保持原样
            kv_storage(const kv_storage& other)
                : key{other.key} {
                if(*this){
                    if constexpr (std::is_trivially_copy_constructible_v<mapped_type>){
                        value = other.value;
                    }else{
                        this->emplace(*other.value.data());
                    }
                }
            }

            kv_storage(kv_storage&& other) noexcept
                : key{std::exchange(other.key, getStaticEmptyKey())} {
                if(*this){
                    if constexpr (std::is_trivially_move_constructible_v<mapped_type>){
                        value = other.value;
                    }else{
                        this->emplace(std::move(*other.value.data()));
                        other.destroy();
                    }
                }
            }

            kv_storage& operator=(const kv_storage& other) {
                if(this == &other) return *this;
                try_destroy();
                key = other.key;
                if(*this){
                    if constexpr (std::is_trivially_copy_constructible_v<mapped_type>){
                        value = other.value;
                    }else{
                        this->emplace(*other.value.data());
                    }
                }
                return *this;
            }

            kv_storage& operator=(kv_storage&& other) noexcept {
                if(this == &other) return *this;
                try_destroy();
                key = std::exchange(other.key, getStaticEmptyKey());
                if(*this){
                    if constexpr (std::is_trivially_move_constructible_v<mapped_type>){
                        value = other.value;
                    }else{
                        this->emplace(std::move(*other.value.data()));
                        other.destroy();
                    }
                }
                return *this;
            }
        };

    public:
        using size_type = std::size_t;
        using value_type_internal = kv_storage;
        using allocator_type = std::allocator_traits<Allocator>::template rebind_alloc<kv_storage>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using buckets = std::vector<value_type_internal, allocator_type>;

    private:
        static constexpr hasher Hasher{};
        static constexpr key_equal KeyEqualer{};

        struct hm_iterator_sentinel {};

        template <bool addConst = false>
        struct hm_iterator_static {
            using container_type = std::conditional_t<addConst, const fixed_open_addr_hash_map, fixed_open_addr_hash_map>;
            using base_iterator = decltype(std::ranges::begin(std::declval<container_type&>().buckets_));
            using iterator_category = std::forward_iterator_tag;
            using iterator_concept = std::forward_iterator_tag; // C++20
            using value_type = kv_storage;
            using difference_type = std::ptrdiff_t;

            [[nodiscard]] hm_iterator_static() = default;
            constexpr bool operator==(const hm_iterator_static& other) const noexcept = default;
            constexpr auto operator<=>(const hm_iterator_static& other) const noexcept {
                return current <=> other.current;
            }

            constexpr bool operator==(const hm_iterator_sentinel&) const noexcept {
                return current == sentinel;
            }

            constexpr hm_iterator_static& operator++() {
                ++current;
                advance_past_empty();
                return *this;
            }

            constexpr hm_iterator_static operator++(int) {
                auto itr = *this;
                ++(*this);
                return itr;
            }

            auto operator*() const noexcept {
                if constexpr(addConst){
                    return std::pair<const key_type&, const mapped_type&>{current->key, *current->value.data()};
                } else{
                    return std::pair<const key_type&, mapped_type&>{current->key, *current->value.data()};
                }
            }

            constexpr auto operator->() const noexcept {
                return arrow_proxy{ **this };
            }

            constexpr explicit hm_iterator_static(container_type* hm) noexcept :
                current(std::ranges::begin(hm->buckets_)),
                sentinel(std::ranges::end(hm->buckets_)){
                advance_past_empty();
            }

            constexpr hm_iterator_static(container_type* hm, const size_type idx) noexcept :
                current(std::ranges::begin(hm->buckets_) + idx), sentinel(std::ranges::end(hm->buckets_)){}

        private:
            template <bool oAddConst>
            friend struct hm_iterator_static;

            template <bool oAddConst>
            explicit(false) hm_iterator_static(const hm_iterator_static<oAddConst>& other)
                : current(other.current), sentinel(other.sentinel){}

            constexpr void advance_past_empty() noexcept {
                // 优化 3: 使用 sentinel 检查在前
                while(current != sentinel && fixed_open_addr_hash_map::is_empty_key(current->key)){
                     ++current;
                }
            }

            [[nodiscard]] constexpr kv_storage& raw() const noexcept {
                return *current;
            }

            [[nodiscard]] constexpr std::size_t getIdx(const base_iterator begin) const noexcept {
                return std::distance(begin, current);
            }

            base_iterator current{};
            base_iterator sentinel{};

            friend container_type;
        };

    public:
        using iterator = hm_iterator_static<false>;
        using const_iterator = hm_iterator_static<true>;

    public:
        explicit constexpr fixed_open_addr_hash_map(
            const size_type bucket_count = 16,
            const allocator_type& alloc = allocator_type{})
        : buckets_(alloc){
            size_type pot_count = std::bit_ceil(bucket_count);
            if (pot_count < 4) pot_count = 4; // 至少4个，防止太小导致的高冲突

            buckets_resize_init(pot_count);
        }

        constexpr fixed_open_addr_hash_map(const fixed_open_addr_hash_map& other, const size_type bucket_count)
            : fixed_open_addr_hash_map(bucket_count, other.get_allocator()){
            for(auto it = other.begin(); it != other.end(); ++it){
                this->transfer_raw(it.raw());
            }
        }

        constexpr fixed_open_addr_hash_map(fixed_open_addr_hash_map&& other, const size_type bucket_count)
            : fixed_open_addr_hash_map(bucket_count, other.get_allocator()){
            for(auto it = other.begin(); it != other.end(); ++it){
                this->transfer_raw(std::move(it.raw()));
            }
        }

        [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
            return buckets_.get_allocator();
        }

        // Iterators
        [[nodiscard]] constexpr iterator begin() noexcept { return iterator(this); }
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{this}; }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator{this}; }
        [[nodiscard]] constexpr iterator end() noexcept { return iterator(this, buckets_.size()); }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator(this, buckets_.size()); }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator(this, buckets_.size()); }

        // Capacity
        [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
        [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
        [[nodiscard]] constexpr size_type max_size() const noexcept { return buckets_.max_size() / 2; }

        // Modifiers
        constexpr void clear() noexcept {
            if (size_ == 0) return;
            for(auto& b : buckets_){
                if(!fixed_open_addr_hash_map::is_empty_key(b.key)){
                    b.key = empty_key();
                    if constexpr (!std::is_trivially_destructible_v<std::remove_cv_t<mapped_type>>){
                        b.destroy();
                    }
                }
            }
            size_ = 0;
        }

        constexpr void seemly_clear() noexcept requires (std::is_trivially_destructible_v<mapped_type> && std::is_copy_assignable_v<key_type>){
            if (size_ == 0) return;
            for(auto& b : buckets_){
                if(!fixed_open_addr_hash_map::is_empty_key(b.key)){
                    b.key = empty_key();
                }
            }
            size_ = 0;
        }

        constexpr std::pair<iterator, bool> insert(const value_type& value){
            return this->emplace_impl(value.first, value.second);
        }
        constexpr std::pair<iterator, bool> insert(value_type&& value){
            return this->emplace_impl(value.first, std::move(value.second));
        }

        template <typename... Args>
        constexpr std::pair<iterator, bool> emplace(Args&&... args){
            return this->emplace_impl(std::forward<Args>(args)...);
        }

        constexpr void erase(const iterator it) { this->erase_impl(it); }
        constexpr size_type erase(const key_type& key) { return this->erase_impl(key); }
        template <typename K> constexpr size_type erase(const K& x) { return this->erase_impl(x); }

        // Lookup
        constexpr mapped_type& at(const key_type& key) { return this->at_impl(key); }
        template <typename K> constexpr mapped_type& at(const K& x) { return this->at_impl(x); }

        [[nodiscard]] constexpr const mapped_type& at(const key_type& key) const { return this->at_impl(key); }
        template <typename K> [[nodiscard]] constexpr const mapped_type& at(const K& x) const { return this->at_impl(x); }

        constexpr mapped_type& operator[](const key_type& key) requires (std::is_default_constructible_v<mapped_type>){
            return this->emplace_impl(key).first->second;
        }

        [[nodiscard]] constexpr size_type count(const key_type& key) const noexcept { return this->count_impl(key); }
        template <typename K> [[nodiscard]] constexpr bool contains(const K& key) const noexcept { return this->find_impl(key) != end(); }

        template <typename... Args>
            requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
        constexpr std::pair<iterator, bool> insert_or_assign(key_type&& key, Args&&... args){
            if(iterator itr = this->find_impl(key); itr != end()){
                itr->second = mapped_type{std::forward<Args>(args)...};
                return std::pair{itr, false};
            } else{
                return this->template emplace_impl<true>(std::move(key), std::forward<Args>(args)...);
            }
        }

        // ... (省略部分 insert_or_assign/try_emplace 重载，逻辑同上，保持不变即可)
        template <typename... Args>
            requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
        constexpr std::pair<iterator, bool> insert_or_assign(const key_type& key, Args&&... args){
            if(iterator itr = this->find_impl(key); itr != end()){
                itr->second = mapped_type{std::forward<Args>(args)...};
                return std::pair{itr, false};
            } else{
                return this->template emplace_impl<true>(key, std::forward<Args>(args)...);
            }
        }

        template <typename... Args>
            requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
        constexpr std::pair<iterator, bool> try_emplace(key_type&& key, Args&&... args){
            if(auto itr = this->find_impl(key); itr != end()){
                return std::pair{itr, false};
            }
            return this->template emplace_impl<true>(std::move(key), std::forward<Args>(args)...);
        }

        template <typename... Args>
            requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
        constexpr std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args){
            if(auto itr = this->find_impl(key); itr != end()){
                return std::pair{itr, false};
            }
            return this->template emplace_impl<true>(key, std::forward<Args>(args)...);
        }

        template <typename K>
        [[nodiscard]] constexpr size_type count(const K& x) const noexcept {
            return this->count_impl(x);
        }

        [[nodiscard]] constexpr iterator find(const key_type& key) noexcept { return this->find_impl(key); }
        template <typename K> [[nodiscard]] constexpr iterator find(const K& x) noexcept { return this->find_impl(x); }

        [[nodiscard]] constexpr const_iterator find(const key_type& key) const noexcept { return this->find_impl(key); }
        template <typename K> [[nodiscard]] constexpr const_iterator find(const K& x) const noexcept { return this->find_impl(x); }

        // Bucket interface
        [[nodiscard]] constexpr size_type bucket_count() const noexcept { return buckets_.size(); }
        [[nodiscard]] constexpr size_type max_bucket_count() const noexcept { return buckets_.max_size(); }

        // Hash policy
        constexpr void rehash(size_type count){
            count = std::max(count, size_type(size() / MAX_LOAD_FACTOR) + 1);
            fixed_open_addr_hash_map other(std::move(*this), count); // count 会被构造函数 bit_ceil
            std::swap(*this, other);
        }

        constexpr void reserve(const size_type count){
            // 优化 4: 仅当需要容纳的 count 超过 max_load 时才 rehash
            if(count > max_load_count_){
                rehash(count);
            }
        }

        constexpr void reserve_exact(const size_type count){
             if(count > buckets_.size()){
                rehash(count);
            }
        }

        // Observers
        [[nodiscard]] constexpr hasher hash_function() const noexcept { return Hasher; }
        [[nodiscard]] constexpr key_equal key_eq() const noexcept { return KeyEqualer; }

        [[nodiscard]] static constexpr decltype(auto) empty_key() noexcept {
            return std::invoke(key_convertor, fixed_empty_key);
        }

    protected:
        // 辅助：初始化 buckets 并计算 max_load_count_
        constexpr void buckets_resize_init(size_type pot_count) {
            if constexpr (std::is_copy_constructible_v<mapped_type>){
                buckets_.resize(pot_count, key_type{empty_key()});
            }else{
                buckets_.clear(); // 安全
                buckets_.reserve(pot_count);
                for(size_type i = 0; i < pot_count; ++i){
                    buckets_.emplace_back(empty_key());
                }
            }
            // 更新缓存的负载阈值
            max_load_count_ = static_cast<size_type>(pot_count * MAX_LOAD_FACTOR);
        }

        template <bool assert_no_equal = false, std::convertible_to<key_type> K, typename... Args>
            requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
        constexpr std::pair<iterator, bool> emplace_impl(K&& key, Args&&... args){
            if(fixed_open_addr_hash_map::is_empty_key(key)){
                throw bad_hash_map_key{};
            }

            // 预先检查是否需要扩容
            if (size_ + 1 > max_load_count_) [[unlikely]] {
                rehash(size_ + 1);
            }

            const size_type mask = buckets_.size() - 1;
            for(std::size_t idx = this->key_to_idx(key, mask);; idx = (idx + 1) & mask){
                auto& kv = buckets_[idx];
                if(fixed_open_addr_hash_map::is_empty_key(kv.key)) [[likely]] {
                    kv.emplace(std::forward<Args>(args)...);
                    kv.key = std::forward<K>(key);
                    size_++;
                    return {iterator(this, idx), true};
                }

                if constexpr (!assert_no_equal){
                    if(KeyEqualer(kv.key, key)){
                        return {iterator(this, idx), false};
                    }
                }
            }
        }

        constexpr void transfer_raw(const kv_storage& insert_kv){
            assert(!fixed_open_addr_hash_map::is_empty_key(insert_kv.key) && "empty key shouldn't be used");

            if (size_ + 1 > max_load_count_) [[unlikely]] {
                // 这里可能需要特殊处理，因为是在构造中，但调用 rehash 安全
                rehash(size_ + 1);
            }

            const size_type mask = buckets_.size() - 1;
            for(std::size_t idx = this->key_to_idx(insert_kv.key, mask);; idx = (idx + 1) & mask){
                auto& kv = buckets_[idx];
                if(fixed_open_addr_hash_map::is_empty_key(kv.key)){
                    kv = insert_kv;
                    size_++;
                    return;
                } else if(KeyEqualer(kv.key, insert_kv.key)){
                    return;
                }
            }
        }

        constexpr void transfer_raw(kv_storage&& insert_kv){
            assert(!fixed_open_addr_hash_map::is_empty_key(insert_kv.key) && "empty key shouldn't be used");

             if (size_ + 1 > max_load_count_) [[unlikely]] {
                rehash(size_ + 1);
            }

            const size_type mask = buckets_.size() - 1;
            for(std::size_t idx = this->key_to_idx(insert_kv.key, mask);; idx = (idx + 1) & mask){
                auto& kv = buckets_[idx];
                if(fixed_open_addr_hash_map::is_empty_key(kv.key)){
                    kv = std::move(insert_kv);
                    size_++;
                    return;
                } else if(KeyEqualer(kv.key, insert_kv.key)){
                    return;
                }
            }
        }

        constexpr void erase_impl(const iterator it) noexcept {
            std::size_t bucket = it.getIdx(buckets_.begin());
            const size_type mask = buckets_.size() - 1;

            buckets_[bucket].key = empty_key();
            buckets_[bucket].destroy();
            size_--;

            std::size_t next_idx = (bucket + 1) & mask;
            while (true) {
                auto& next_kv = buckets_[next_idx];
                if (fixed_open_addr_hash_map::is_empty_key(next_kv.key)) [[likely]] {
                    return;
                }

                std::size_t ideal = this->key_to_idx(next_kv.key, mask);

                // 优化 5: 提取 diff 逻辑，虽然逻辑没变，但配合 bit mask 确保正确
                // diff(ideal, bucket) < diff(ideal, next_idx)
                // 意味着 bucket 在 next_idx 的理想位置和 next_idx 当前位置之间
                if (((bucket - ideal) & mask) < ((next_idx - ideal) & mask)) {
                    buckets_[bucket] = std::move(next_kv);
                    bucket = next_idx;
                }
                next_idx = (next_idx + 1) & mask;
            }
        }

        template <typename K>
        constexpr size_type erase_impl(const K& key) noexcept {
            // 这里不能直接复用 find_impl，因为我们需要 non-const iterator
            // 而 find_impl 返回类型取决于是否 const
            auto it = this->find(key);
            if(it != end()){
                this->erase_impl(it);
                return 1;
            }
            return 0;
        }

        template <typename K>
        constexpr mapped_type& at_impl(const K& key){
            const auto it = this->find_impl(key);
            if(it != end()) [[likely]] {
                return it->second;
            }
            throw std::out_of_range("HashMap::at");
        }

        template <typename K>
        constexpr const mapped_type& at_impl(const K& key) const {
            return const_cast<fixed_open_addr_hash_map*>(this)->at_impl(key);
        }

        template <typename K>
        constexpr std::size_t count_impl(const K& key) const noexcept {
            return this->find_impl(key) == end() ? 0 : 1;
        }

        template <typename K>
        constexpr iterator find_impl(const K& key) noexcept {
            static_assert(isEqualerValid<K>, "invalid equaler");
            if(fixed_open_addr_hash_map::is_empty_key(key)) return end();

            const size_type mask = buckets_.size() - 1;
            for(std::size_t idx = this->key_to_idx(key, mask);; idx = (idx + 1) & mask){
                const auto& tKey = buckets_[idx].key;
                if(KeyEqualer(tKey, key)) [[likely]] {
                    return iterator(this, idx);
                }

                if(fixed_open_addr_hash_map::is_empty_key(tKey)) [[likely]] {
                    return end();
                }
            }
        }

        template <typename K>
        constexpr const_iterator find_impl(const K& key) const noexcept {
            return const_cast<fixed_open_addr_hash_map*>(this)->find_impl(key);
        }

        // 核心优化：Hash Mixing + Bit Mask
        template <typename K>
        constexpr std::size_t key_to_idx(const K& key, size_type mask) const noexcept(noexcept(Hasher(key))){
            static_assert(isHasherValid<K>, "invalid hasher");
            // 在 Mask 之前进行 Mix，打散比特位
            return hash_mixer::mix(Hasher(key)) & mask;
        }

        // probe_next 移除，直接内联 `(idx + 1) & mask` 更清晰

    private:
        buckets buckets_{};
        std::size_t size_{};
        std::size_t max_load_count_{}; // 新增：缓存 resize 阈值

        static constexpr Convertor key_convertor{};
        static constexpr EmptyKey fixed_empty_key{[]() constexpr {
            if constexpr (std::same_as<std::in_place_t, decltype(emptyKey)>){
                return EmptyKey();
            }else{
                return emptyKey;
            }
        }()};
    };

    export
    template <
        typename Key, typename Val, auto emptyKey = std::in_place,
        typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>,
        std::invocable<const decltype(emptyKey)&> Convertor = std::identity,
        typename Allocator = std::allocator<std::pair<Key, Val>>>
    using fixed_open_hash_map = fixed_open_addr_hash_map<
        Key, Val, std::conditional_t<std::same_as<decltype(emptyKey), std::in_place_t>, Key, decltype(emptyKey)>, emptyKey, Hash, KeyEqual, Convertor, Allocator>;

    export
    template <
        typename Key, typename Val,
        typename Hash = pointer_transparent_hasher,
        typename KeyEqual = std::equal_to<void>,
        typename Allocator = std::allocator<std::pair<Key, Val>>>
    using pointer_hash_map = fixed_open_addr_hash_map<
        Key, Val, std::nullptr_t, nullptr, Hash, KeyEqual, std::identity, Allocator>;
}
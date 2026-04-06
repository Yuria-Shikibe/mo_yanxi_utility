export module mo_yanxi.concurrent.mpsc_double_buffer;

import std;

namespace mo_yanxi::ccur {

export class mpsc_double_buffer_sync {
    alignas(std::hardware_destructive_interference_size) mutable std::atomic<std::uint32_t> state_{0};

    static constexpr std::uint32_t INDEX_BIT = 1 << 0;
    static constexpr std::uint32_t LOCK_BIT = 1 << 1;
    static constexpr std::uint32_t HAS_DATA_BIT = 1 << 2;

public:
    constexpr mpsc_double_buffer_sync() noexcept = default;

    [[nodiscard]] std::uint32_t active_index() const noexcept {
        return state_.load(std::memory_order_relaxed) & INDEX_BIT;
    }

    [[nodiscard]] bool has_data() const noexcept {
        return (state_.load(std::memory_order_acquire) & HAS_DATA_BIT) != 0;
    }

    std::uint32_t lock() const noexcept {
        std::uint32_t current = state_.load(std::memory_order_relaxed);
        while(true) {
            if((current & LOCK_BIT) == 0) {
                if(state_.compare_exchange_weak(current, current | LOCK_BIT,
                    std::memory_order_acquire,
                    std::memory_order_relaxed)) {
                    return current & INDEX_BIT;
                }
            } else {
                state_.wait(current, std::memory_order_relaxed);
                current = state_.load(std::memory_order_relaxed);
            }
        }
    }

    void unlock() const noexcept {
        std::uint32_t current = state_.load(std::memory_order_relaxed);
        state_.store(current & ~LOCK_BIT, std::memory_order_release);
        state_.notify_one();
    }

    void unlock_and_set_data() const noexcept {
        std::uint32_t current = state_.load(std::memory_order_relaxed);
        std::uint32_t new_state = (current & ~LOCK_BIT) | HAS_DATA_BIT;
        state_.store(new_state, std::memory_order_release);
        state_.notify_one();
    }

    void flip_and_unlock() const noexcept {
        std::uint32_t current = state_.load(std::memory_order_relaxed);
        std::uint32_t new_state = (current ^ INDEX_BIT) & ~(HAS_DATA_BIT | LOCK_BIT);
        state_.store(new_state, std::memory_order_release);
        state_.notify_one();
    }

    template <bool SetData = true, typename F>
    constexpr decltype(auto) lock_do_unlock(F&& f) const {
        std::uint32_t idx = lock();

        if constexpr (noexcept(std::invoke(std::forward<F>(f), idx))) {
            if constexpr (std::is_void_v<std::invoke_result_t<F, std::uint32_t>>) {
                std::invoke(std::forward<F>(f), idx);
                if constexpr (SetData) unlock_and_set_data();
                else unlock();
            } else {
                decltype(auto) ret = std::invoke(std::forward<F>(f), idx);
                if constexpr (SetData) unlock_and_set_data();
                else unlock();
                return ret;
            }
        } else {
            try {
                if constexpr (std::is_void_v<std::invoke_result_t<F, std::uint32_t>>) {
                    std::invoke(std::forward<F>(f), idx);
                    if constexpr (SetData) unlock_and_set_data();
                    else unlock();
                } else {
                    decltype(auto) ret = std::invoke(std::forward<F>(f), idx);
                    if constexpr (SetData) unlock_and_set_data();
                    else unlock();
                    return ret;
                }
            } catch (...) {
                unlock();
                throw;
            }
        }
    }
};

template <typename C, typename T>
concept double_buffer_container = true;

export
template <typename T, double_buffer_container<T> Container = std::vector<T>>
class mpsc_double_buffer {
    static_assert(std::is_object_v<T>);
public:
    using container_type = Container;
    using allocator_type = typename container_type::allocator_type;
    using value_type = T;

    static constexpr bool is_nothrow_copy_constructible =
        std::is_nothrow_copy_constructible_v<container_type>;

    static constexpr bool is_nothrow_alloc_constructible =
        std::is_nothrow_constructible_v<container_type, const allocator_type&>;

    static constexpr bool is_nothrow_push_back =
        requires(container_type& c, const T& item) { { c.push_back(item) } noexcept; };

    static constexpr bool is_nothrow_move_push_back =
        requires(container_type& c, T&& item) { { c.push_back(std::move(item)) } noexcept; };

    template <typename... Args>
    static constexpr bool is_nothrow_emplace =
        requires(container_type& c, Args&&... args) { { c.emplace_back(std::forward<Args>(args)...) } noexcept; };

    template <typename Fn>
    static constexpr bool is_nothrow_modify =
        std::is_nothrow_invocable_v<Fn, container_type&>;

    explicit mpsc_double_buffer(const container_type& init_container)
        noexcept(is_nothrow_copy_constructible)
        requires (std::copy_constructible<T>)
        : buffers_{init_container, init_container} {
    }

    explicit mpsc_double_buffer(const allocator_type& alloc)
        noexcept(is_nothrow_alloc_constructible)
        : buffers_{container_type{alloc}, container_type{alloc}} {
    }

    [[nodiscard]] mpsc_double_buffer() = default;

    container_type* fetch() noexcept {
        if (!sync_.has_data()) {
            return nullptr;
        }

        std::uint32_t current_idx = sync_.active_index();
        std::uint32_t inactive_idx = 1 - current_idx;
        buffers_[inactive_idx].clear();

        (void)sync_.lock();
        sync_.flip_and_unlock();

        return std::addressof(buffers_[current_idx]);
    }

    void push(const T& item) noexcept(is_nothrow_push_back) {
        sync_.lock_do_unlock<true>([&](std::uint32_t idx) noexcept(is_nothrow_push_back) {
            buffers_[idx].push_back(item);
        });
    }

    void push(T&& item) noexcept(is_nothrow_move_push_back) {
        sync_.lock_do_unlock<true>([&](std::uint32_t idx) noexcept(is_nothrow_move_push_back) {
            buffers_[idx].push_back(std::move(item));
        });
    }

    template <typename ...Args>
        requires std::constructible_from<T, Args&&...>
    void emplace(Args&& ...args) noexcept(is_nothrow_emplace<Args...>) {
        sync_.lock_do_unlock<true>([&](std::uint32_t idx) noexcept(is_nothrow_emplace<Args...>) {
            buffers_[idx].emplace_back(std::forward<Args>(args)...);
        });
    }

    template <std::invocable<container_type&> Fn>
    void modify(Fn&& fn) noexcept(is_nothrow_modify<Fn>) {
        sync_.lock_do_unlock<true>([&](std::uint32_t idx) noexcept(is_nothrow_modify<Fn>) {
            std::invoke(std::forward<Fn>(fn), buffers_[idx]);
        });
    }

    std::size_t size() const noexcept requires(std::ranges::sized_range<container_type>) {
        return sync_.lock_do_unlock<false>([&](std::uint32_t idx) noexcept(noexcept(std::ranges::size(buffers_[idx]))) {
            return std::ranges::size(buffers_[idx]);
        });
    }

private:
    alignas(std::hardware_destructive_interference_size) container_type buffers_[2];
    mpsc_double_buffer_sync sync_;
};

export
template <typename T, double_buffer_container<T> C>
struct mpsc_double_buffer_no_propagate : mpsc_double_buffer<T, C> {
    using mpsc_double_buffer<T, C>::mpsc_double_buffer;

    mpsc_double_buffer_no_propagate() = default;

    mpsc_double_buffer_no_propagate(const mpsc_double_buffer_no_propagate& other) noexcept
        : mpsc_double_buffer<T, C>() {}

    mpsc_double_buffer_no_propagate(mpsc_double_buffer_no_propagate&& other) noexcept
        : mpsc_double_buffer<T, C>() {}

    mpsc_double_buffer_no_propagate& operator=(const mpsc_double_buffer_no_propagate& other) noexcept {
        return *this;
    }
    mpsc_double_buffer_no_propagate& operator=(mpsc_double_buffer_no_propagate&& other) noexcept {
        return *this;
    }
};

export
template <typename Container>
class mpsc_double_buffer_heterogeneous {
public:
    using container_type = Container;

    // 1. 默认构造函数（如果底层容器支持）
    [[nodiscard]] mpsc_double_buffer_heterogeneous() requires std::default_initializable<container_type> = default;

    // 2. 拷贝构造函数：从现有的容器初始化
    explicit mpsc_double_buffer_heterogeneous(const container_type& init_container)
        noexcept(std::is_nothrow_copy_constructible_v<container_type>)
        requires std::copy_constructible<container_type>
        : buffers_{init_container, init_container} {
    }

    // 3. 鸭子类型的 Allocator 构造：无需强制容器提供 allocator_type
    // 只要该容器可以由 Alloc 类型构造，此函数就生效
    template <typename Alloc>
        requires requires(const Alloc& alloc) { container_type{alloc}; }
    explicit mpsc_double_buffer_heterogeneous(const Alloc& alloc)
        noexcept(std::is_nothrow_constructible_v<container_type, const Alloc&>)
        : buffers_{container_type{alloc}, container_type{alloc}} {
    }

    // 获取并翻转缓冲区（鸭子类型：仅要求支持 clear()）
    container_type* fetch() noexcept requires requires(container_type& c) { c.clear(); } {
        if (!sync_.has_data()) {
            return nullptr;
        }

        std::uint32_t current_idx = sync_.active_index();
        std::uint32_t inactive_idx = 1 - current_idx;

        buffers_[inactive_idx].clear();

        (void)sync_.lock();
        sync_.flip_and_unlock();

        return std::addressof(buffers_[current_idx]);
    }

    // 鸭子类型 push_back：完美转发任意参数到底层的 push_back
    template <typename... Args>
        requires requires(container_type& c, Args&&... args) { c.push_back(std::forward<Args>(args)...); }
    void push_back(Args&&... args) noexcept(noexcept(std::declval<container_type&>().push_back(std::forward<Args>(args)...))) {
        sync_.template lock_do_unlock<true>([&](std::uint32_t idx) noexcept(noexcept(buffers_[idx].push_back(std::forward<Args>(args)...))) {
            buffers_[idx].push_back(std::forward<Args>(args)...);
        });
    }

    // 鸭子类型 emplace_back：完美转发任意参数到底层的 emplace_back
    template <typename... Args>
        requires requires(container_type& c, Args&&... args) { c.emplace_back(std::forward<Args>(args)...); }
    void emplace_back(Args&&... args) noexcept(noexcept(std::declval<container_type&>().emplace_back(std::forward<Args>(args)...))) {
        sync_.template lock_do_unlock<true>([&](std::uint32_t idx) noexcept(noexcept(buffers_[idx].emplace_back(std::forward<Args>(args)...))) {
            buffers_[idx].emplace_back(std::forward<Args>(args)...);
        });
    }

    // Modify：传入回调对当前容器进行直接操作
    template <std::invocable<container_type&> Fn>
    void modify(Fn&& fn) noexcept(std::is_nothrow_invocable_v<Fn, container_type&>) {
        sync_.template lock_do_unlock<true>([&](std::uint32_t idx) noexcept(std::is_nothrow_invocable_v<Fn, container_type&>) {
            std::invoke(std::forward<Fn>(fn), buffers_[idx]);
        });
    }

    // 鸭子类型 size：只有底层容器支持 size() 或 std::ranges::size 时才暴露此接口
    auto size() const noexcept(noexcept(std::declval<const container_type&>().size()))
        requires requires(const container_type& c) { c.size(); }
    {
        return sync_.template lock_do_unlock<false>([&](std::uint32_t idx) noexcept(noexcept(buffers_[idx].size())) {
            return buffers_[idx].size();
        });
    }

private:
    alignas(std::hardware_destructive_interference_size) container_type buffers_[2];
    mpsc_double_buffer_sync sync_;
};


}
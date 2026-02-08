export module mo_yanxi.concurrent.atomic_double_buffer;

import std;

namespace mo_yanxi::ccur {
    export
    template <typename T>
    struct atomic_double_buffer {
        using value_type = T;
    private:
        using atomic_value_type = unsigned;
        static constexpr unsigned index_bit = 0b1;
        static constexpr unsigned load_bit  = 0b10;
        static constexpr unsigned store_bit = 0b100;

        static constexpr unsigned wait_load_bit = 0b1000;
        static constexpr unsigned wait_store_bit = 0b10000;

        std::array<value_type, 2> buffer{};
        std::atomic<atomic_value_type> flags{};

    public:
        template <std::invocable<value_type&> Modifier>
        void modify(Modifier modifier) noexcept(std::is_nothrow_invocable_v<Modifier, value_type&>) {
            // mark modify is wanted
            const auto cur = flags.fetch_or(store_bit, std::memory_order_acquire);
            const auto last_idx = cur & index_bit;
            const auto next_idx = last_idx ^ index_bit;

            if constexpr (noexcept(std::is_nothrow_invocable_v<Modifier, value_type&>)) {
                std::invoke(std::move(modifier), buffer[next_idx]);
            } else {
                try {
                    std::invoke(std::move(modifier), buffer[next_idx]);
                } catch (...) {
                    // 异常退出时，如果有 Reader 在等待 store 结束，需要唤醒
                    // 同时清除 store_bit 和 wait_store_bit
                    auto prev = flags.fetch_and(~(store_bit | wait_store_bit), std::memory_order_release);
                    if (prev & wait_store_bit) {
                        flags.notify_one();
                    }
                    throw;
                }
            }

            auto expected = last_idx | store_bit;
            while (true) {
                // 尝试原子地 移除 store_bit 并 翻转 index
                // 注意：此时 flags 中可能包含 wait_store_bit（Reader 在等我们写完），
                // compare_exchange_weak 会处理这种情况（如果 expected 不包含 wait_store_bit，它会失败并更新 expected）

                // 只有当没有 load_bit 时，我们才允许提交
                if (!(expected & load_bit)) {
                    // 尝试提交：切换到 next_idx，同时清除 store_bit 和 wait_store_bit
                    if (flags.compare_exchange_weak(expected, next_idx,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {

                        // 成功！检查之前是否有 Reader 设置了等待位
                        if (expected & wait_store_bit) {
                            flags.notify_one();
                        }
                        break;
                    }
                    // CAS 失败，expected 被更新，继续循环
                } else {
                    // 走到这里说明 (expected & load_bit) 为真，有消费者在读

                    // 1. 我们需要设置 wait_load_bit 告诉消费者：我在等你
                    if (!(expected & wait_load_bit)) {
                        if (flags.compare_exchange_weak(expected, expected | wait_load_bit,
                            std::memory_order_relaxed)) {
                            expected |= wait_load_bit; // 本地更新状态
                        } else {
                            // CAS 失败（可能 load 结束了，或者状态变了），重新循环检查
                            continue;
                        }
                    }

                    // 2. 等待 load 结束
                    // 此时 expected 包含 load_bit | wait_load_bit
                    flags.wait(expected, std::memory_order_relaxed);

                    // 3. 唤醒后重新加载状态
                    expected = flags.load(std::memory_order_relaxed);
                }
            }
        }

        void store(value_type&& value) noexcept(std::is_nothrow_move_assignable_v<value_type>) {
            this->modify([&](value_type& t) {
                t = std::move(value);
            });
        }

        void store(const value_type& value) noexcept(std::is_nothrow_copy_assignable_v<value_type>) {
            this->modify([&](value_type& t) {
                t = value;
            });
        }

        template <std::invocable<value_type&> Loader>
        void load(Loader loader) noexcept(std::is_nothrow_invocable_v<Loader, value_type&>) {
            const auto cur = flags.fetch_or(load_bit, std::memory_order_acquire);
            const auto last_idx = cur & index_bit;

            if constexpr (noexcept(std::is_nothrow_invocable_v<Loader, value_type&>)) {
                std::invoke(std::move(loader), buffer[last_idx]);
            } else {
                try {
                    std::invoke(std::move(loader), buffer[last_idx]);
                } catch (...) {
                    _unlock_load_and_notify();
                    throw;
                }
            }

            _unlock_load_and_notify();
        }

        template <std::invocable<value_type&> Loader>
        void load_latest(Loader loader) noexcept(std::is_nothrow_invocable_v<Loader, value_type&>) {
            // 这是一个优化循环：等待直到没有 store 操作，并获取最新的数据权限
            auto expected = flags.load(std::memory_order_acquire);

            while (true) {
                if (expected & store_bit) {
                    // 正在写入，我们需要等待
                    // 1. 设置 wait_store_bit
                    if (!(expected & wait_store_bit)) {
                        if (flags.compare_exchange_weak(expected, expected | wait_store_bit, std::memory_order_relaxed)) {
                            expected |= wait_store_bit;
                        } else {
                            continue; // 状态变了，重试
                        }
                    }

                    // 2. 等待 store 结束
                    flags.wait(expected, std::memory_order_relaxed);
                    expected = flags.load(std::memory_order_acquire);
                } else {
                    // 没有写入，尝试获取 load 锁 (设置 load_bit)
                    // 注意：我们要保留 index_bit，清除可能残留的 wait_bits (虽然理论上 store 结束时会清掉 wait_store)
                    // 但这里主要关注设置 load_bit
                    if (flags.compare_exchange_weak(expected, expected | load_bit, std::memory_order_acquire, std::memory_order_relaxed)) {
                        break; // 获取成功
                    }
                    // CAS 失败，expected 更新，重试
                }
            }

            // 成功获取 load 锁
            const auto last_idx = expected & index_bit;

            if constexpr (noexcept(std::is_nothrow_invocable_v<Loader, value_type&>)) {
                std::invoke(std::move(loader), buffer[last_idx]);
            } else {
                try {
                    std::invoke(std::move(loader), buffer[last_idx]);
                } catch (...) {
                    _unlock_load_and_notify();
                    throw;
                }
            }

            _unlock_load_and_notify();
        }

    private:
        // 辅助函数：退出 load 状态并按需通知
        void _unlock_load_and_notify() noexcept {
            // 退出时，清除 load_bit。
            // 同时，如果 wait_load_bit 被设置了，我们也一并清除它，因为我们要去唤醒 Writer 了。
            // 这样 Writer 醒来后看到的 flag 比较干净。
            auto prev = flags.fetch_and(~(load_bit | wait_load_bit), std::memory_order_release);

            // 只有当之前确实有 Writer 标记了 wait_load_bit 时，才调用系统调用
            if (prev & wait_load_bit) {
                flags.notify_one();
            }
        }
    };

    export
    template <typename T>
    struct atomic_double_buffer_no_propagate : atomic_double_buffer<T> {
        atomic_double_buffer_no_propagate() = default;
        atomic_double_buffer_no_propagate(const atomic_double_buffer_no_propagate& other) noexcept {}
        atomic_double_buffer_no_propagate(atomic_double_buffer_no_propagate&& other) noexcept {}
        atomic_double_buffer_no_propagate& operator=(const atomic_double_buffer_no_propagate& other) noexcept {
            return *this;
        }
        atomic_double_buffer_no_propagate& operator=(atomic_double_buffer_no_propagate&& other) noexcept {
            return *this;
        }
    };
}
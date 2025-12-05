export module mo_yanxi.ext.array_queue;

import std;

export namespace mo_yanxi {
    template <typename T, std::size_t capacity>
        requires (std::is_default_constructible_v<T>)
    class array_queue {
    public:
        [[nodiscard]] constexpr array_queue() = default;

        [[nodiscard]] constexpr bool empty() const noexcept {
            return count == 0;
        }

        [[nodiscard]] constexpr bool full() const noexcept {
            return count == capacity;
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return count;
        }

        [[nodiscard]] constexpr std::size_t max_size() const noexcept {
            return capacity;
        }

        [[nodiscard]] constexpr T& front() {
            if (empty()) {
                throw std::underflow_error("Queue is empty");
            }
            return data[frontIndex];
        }

        [[nodiscard]] constexpr const T& front() const {
            if (empty()) {
                throw std::underflow_error("Queue is empty");
            }
            return data[frontIndex];
        }

        [[nodiscard]] constexpr T& back() {
            if (empty()) {
                throw std::underflow_error("Queue is empty");
            }
            return data[(backIndex == 0 ? capacity : backIndex) - 1];
        }

        [[nodiscard]] constexpr const T& back() const {
            if (empty()) {
                throw std::underflow_error("Queue is empty");
            }
            return data[(backIndex == 0 ? capacity : backIndex) - 1];
        }

        T& operator[](const std::size_t index) noexcept {
            return data[(frontIndex + index) % capacity];
        }

        const T& operator[](const std::size_t index) const noexcept {
            return data[(frontIndex + index) % capacity];
        }

        constexpr void push_back(const T& item) noexcept(std::is_nothrow_copy_assignable_v<T>)
            requires (std::is_copy_assignable_v<T>)
        {
#if MO_YANXI_UTILITY_ENABLE_CHECK
            if (full()) {
                throw std::overflow_error("Queue is full");
            }
#endif
            data[backIndex] = item;
            backIndex = (backIndex + 1) % capacity;
            ++count;
        }

        constexpr void push_back(T&& item) noexcept(std::is_nothrow_move_assignable_v<T>)
            requires (std::is_move_assignable_v<T>)
        {
#if MO_YANXI_UTILITY_ENABLE_CHECK
            if (full()) {
                throw std::overflow_error("Queue is full");
            }
#endif
            data[backIndex] = std::move(item);
            backIndex = (backIndex + 1) % capacity;
            ++count;
        }

        template <typename... Args>
            requires (std::constructible_from<T, Args...>)
        constexpr T& emplace_back(Args&&... args) noexcept(noexcept(T{ std::forward<Args>(args)... }))
        {
#if MO_YANXI_UTILITY_ENABLE_CHECK
            if (full()) {
                throw std::overflow_error("Queue is full");
            }
#endif
            T& rst = (data[backIndex] = T{ std::forward<Args>(args)... });
            backIndex = (backIndex + 1) % capacity;
            ++count;
            return rst;
        }

        constexpr void push_front(const T& item) noexcept(std::is_nothrow_copy_assignable_v<T>)
            requires (std::is_copy_assignable_v<T>)
        {
#if MO_YANXI_UTILITY_ENABLE_CHECK
            if (full()) {
                throw std::overflow_error("Queue is full");
            }
#endif
            frontIndex = (frontIndex == 0) ? capacity - 1 : frontIndex - 1;
            data[frontIndex] = item;
            ++count;
        }

        constexpr void push_front(T&& item) noexcept(std::is_nothrow_move_assignable_v<T>)
            requires (std::is_move_assignable_v<T>)
        {
#if MO_YANXI_UTILITY_ENABLE_CHECK
            if (full()) {
                throw std::overflow_error("Queue is full");
            }
#endif
            frontIndex = (frontIndex == 0) ? capacity - 1 : frontIndex - 1;
            data[frontIndex] = std::move(item);
            ++count;
        }

        template <typename... Args>
            requires (std::constructible_from<T, Args...>)
        constexpr T& emplace_front(Args&&... args) noexcept(noexcept(T{ std::forward<Args>(args)... }))
        {
#if MO_YANXI_UTILITY_ENABLE_CHECK
            if (full()) {
                throw std::overflow_error("Queue is full");
            }
#endif
            frontIndex = (frontIndex == 0) ? capacity - 1 : frontIndex - 1;
            T& rst = (data[frontIndex] = T{ std::forward<Args>(args)... });
            ++count;
            return rst;
        }

        constexpr void pop_front() {
            if (empty()) {
                throw std::underflow_error("Queue is empty");
            }
            frontIndex = (frontIndex + 1) % capacity;
            --count;
        }

        constexpr void pop_back() {
            if (empty()) {
                throw std::underflow_error("Queue is empty");
            }
            backIndex = (backIndex == 0) ? capacity - 1 : backIndex - 1;
            --count;
        }

        constexpr void push_back_and_replace(const T& element) noexcept(std::is_nothrow_copy_assignable_v<T>) {
            if (full()) {
                pop_front();
            }
            this->push_back(element);
        }

        constexpr void push_back_and_replace(T&& element) noexcept(std::is_nothrow_move_assignable_v<T>) {
            if (full()) {
                pop_front();
            }
            this->push_back(std::move(element));
        }

        template <typename... Args>
        constexpr decltype(auto) emplace_back_and_replace(Args&&... args) noexcept(noexcept(T{ std::forward<Args>(args)... })) {
            if (full()) {
                pop_front();
            }
            return this->emplace_back(std::forward<Args>(args)...);
        }

    private:
        std::array<T, capacity> data{};
        std::size_t frontIndex{};
        std::size_t backIndex{};
        std::size_t count{};
    };
}
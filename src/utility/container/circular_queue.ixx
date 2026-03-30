module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.circular_queue;

import std;

namespace mo_yanxi {
export
template <typename T, bool autoResize = true, std::unsigned_integral SizeType = std::size_t, typename Allocator = std::allocator<T>>
struct circular_queue {
    static constexpr bool auto_resize = autoResize;
	static_assert(std::is_same_v<T, typename std::allocator_traits<Allocator>::value_type>);

    using value_type      = T;
    using allocator_type  = Allocator;
    using size_type       = SizeType;
    using difference_type = std::allocator_traits<allocator_type>::difference_type;
    using pointer         = std::allocator_traits<allocator_type>::pointer;
    using const_pointer   = std::allocator_traits<allocator_type>::const_pointer;
    using reference       = value_type&;
    using const_reference = const value_type&;

    // --- 构造函数与析构函数 ---

    [[nodiscard]] constexpr circular_queue() noexcept(noexcept(Allocator())) : circular_queue(Allocator()) {}

    [[nodiscard]] explicit constexpr circular_queue(const allocator_type& alloc) noexcept : alloc{alloc} {
        this->reserve(8);
    }

    [[nodiscard]] explicit constexpr circular_queue(const size_type length, const allocator_type& alloc = Allocator()) : alloc{alloc} {
        this->reserve(length);
    }

    // 内部帮助构造函数：为给定容量分配空间
    [[nodiscard]] constexpr circular_queue(const circular_queue& other, const size_type length, const allocator_type& a)
        : alloc{a},
          data_{length > 0 ? std::allocator_traits<allocator_type>::allocate(alloc, length) : nullptr},
          head{0},
          tail{(other.size_ == length) ? 0 : other.size_},
          size_{other.size_},
          capacity_{length}
    {
        if (size_ == 0 || length == 0) return;
        size_type first_part_len = std::min(other.size_, other.capacity_ - other.head);

        this->alloc_copy(std::to_address(other.data_) + other.head,
                   std::to_address(other.data_) + other.head + first_part_len,
                   std::to_address(data_));

        if (other.size_ > first_part_len) {
            this->alloc_copy(std::to_address(other.data_),
                       std::to_address(other.data_) + (other.size_ - first_part_len),
                       std::to_address(data_) + first_part_len);
        }
    }

    constexpr circular_queue(const circular_queue& other)
        : circular_queue(other, other.capacity_, std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.alloc)) {}

    constexpr circular_queue(const circular_queue& other, const allocator_type& a)
        : circular_queue(other, other.capacity_, a) {}

    constexpr circular_queue(circular_queue&& other) noexcept
        : alloc{std::move(other.alloc)},
          data_{std::exchange(other.data_, nullptr)},
          head{std::exchange(other.head, 0)},
          tail{std::exchange(other.tail, 0)},
          size_{std::exchange(other.size_, 0)},
          capacity_{std::exchange(other.capacity_, 0)} {}

    constexpr circular_queue(circular_queue&& other, const allocator_type& a) noexcept(std::allocator_traits<allocator_type>::is_always_equal::value)
        : alloc{a}
    {
        if constexpr (std::allocator_traits<allocator_type>::is_always_equal::value) {
            data_ = std::exchange(other.data_, nullptr);
            head = std::exchange(other.head, 0);
            tail = std::exchange(other.tail, 0);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
        } else {
            if (a == other.alloc) {
                data_ = std::exchange(other.data_, nullptr);
                head = std::exchange(other.head, 0);
                tail = std::exchange(other.tail, 0);
                size_ = std::exchange(other.size_, 0);
                capacity_ = std::exchange(other.capacity_, 0);
            } else {
                capacity_ = other.capacity_;
                size_ = other.size_;
                head = 0;
                tail = size_ == capacity_ ? 0 : size_;
                data_ = capacity_ > 0 ? std::allocator_traits<allocator_type>::allocate(alloc, capacity_) : nullptr;

                if (size_ > 0) {
                    size_type first_part_len = std::min(other.size_, other.capacity_ - other.head);
                    this->alloc_move(std::to_address(other.data_) + other.head,
                               std::to_address(other.data_) + other.head + first_part_len,
                               std::to_address(data_));
                    if (other.size_ > first_part_len) {
                        this->alloc_move(std::to_address(other.data_),
                                   std::to_address(other.data_) + (other.size_ - first_part_len),
                                   std::to_address(data_) + first_part_len);
                    }
                }
            }
        }
    }

    constexpr ~circular_queue() {
        destruct();
        if (data_) {
            std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
        }
    }

    // --- 赋值运算符 ---

    constexpr circular_queue& operator=(const circular_queue& other) {
        if (this == &other) return *this;
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value) {
            if (alloc != other.alloc) {
                clear();
                if (data_) std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
                data_ = nullptr;
                capacity_ = 0;
            }
            alloc = other.alloc;
        }
        circular_queue temp(other, alloc);
        std::ranges::swap(data_, temp.data_);
        std::ranges::swap(head, temp.head);
        std::ranges::swap(tail, temp.tail);
        std::ranges::swap(size_, temp.size_);
        std::ranges::swap(capacity_, temp.capacity_);
        return *this;
    }

    constexpr circular_queue& operator=(circular_queue&& other) noexcept(
        std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value ||
        std::allocator_traits<allocator_type>::is_always_equal::value)
    {
        if (this == &other) return *this;
        clear();
        if (data_) {
            std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
        }
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value) {
            alloc = std::move(other.alloc);
        }
        data_ = std::exchange(other.data_, nullptr);
        head = std::exchange(other.head, 0);
        tail = std::exchange(other.tail, 0);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        return *this;
    }

    // --- 分配器访问 ---

    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return alloc;
    }

    // --- 容量与修改 ---

    constexpr void reserve(const size_type newCapacity) {
        if (newCapacity <= capacity_) return;

        pointer newData = std::allocator_traits<allocator_type>::allocate(alloc, newCapacity);

        if (!empty()) {
            size_type first_part_len = std::min(size_, capacity_ - head);
            this->alloc_move(std::to_address(data_) + head,
                       std::to_address(data_) + head + first_part_len,
                       std::to_address(newData));

            if (size_ > first_part_len) {
                this->alloc_move(std::to_address(data_),
                           std::to_address(data_) + (size_ - first_part_len),
                           std::to_address(newData) + first_part_len);
            }

            destruct();
            std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
        } else if (data_) {
            std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
        }

        data_ = newData;
        capacity_ = newCapacity;
        head = 0;
        tail = (size_ == capacity_) ? 0 : size_;
    }

    constexpr void push_back(const value_type& val) noexcept(std::is_nothrow_copy_assignable_v<value_type>) {
        tryExpand();
        std::allocator_traits<allocator_type>::construct(alloc, std::to_address(data_) + tail, val);
        incrTail();
    }

    constexpr void push_back(value_type&& val) noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        tryExpand();
        std::allocator_traits<allocator_type>::construct(alloc, std::to_address(data_) + tail, std::move(val));
        incrTail();
    }

    constexpr void push_front(const value_type& val) noexcept(std::is_nothrow_copy_assignable_v<value_type>) {
        tryExpand();
        decrHead();
        std::allocator_traits<allocator_type>::construct(alloc, std::to_address(data_) + head, val);
    }

    constexpr void push_front(value_type&& val) noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        tryExpand();
        decrHead();
        std::allocator_traits<allocator_type>::construct(alloc, std::to_address(data_) + head, std::move(val));
    }

    template <typename... Args>
        requires (std::constructible_from<value_type, Args...>)
    constexpr value_type& emplace_front(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>) {
        tryExpand();
        decrHead();
        std::allocator_traits<allocator_type>::construct(alloc, std::to_address(data_) + head, std::forward<Args>(args)...);
        return data_[head];
    }

    template <typename... Args>
        requires (std::constructible_from<value_type, Args...>)
    constexpr value_type& emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>) {
        tryExpand();
        std::allocator_traits<allocator_type>::construct(alloc, std::to_address(data_) + tail, std::forward<Args>(args)...);
        value_type& rst = data_[tail];
        incrTail();
        return rst;
    }

    constexpr void pop_back() noexcept {
        assert(!empty());
        --size_;
        tail = get_back_index();
        std::allocator_traits<allocator_type>::destroy(alloc, std::to_address(data_) + tail);
    }

    constexpr void pop_front() noexcept {
        assert(!empty());
        --size_;
        std::allocator_traits<allocator_type>::destroy(alloc, std::to_address(data_) + head);
        ++head;
        if (head == capacity_) {
            head = 0;
        }
    }

    constexpr void clear() noexcept {
        destruct();
        head = 0;
        tail = 0;
        size_ = 0;
    }

    // ...(此处省略 empty, full, size, capacity, front, back 等等未经改变语义的纯查询代码) ...
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] constexpr bool full() const noexcept { return size_ == capacity_; }
    [[nodiscard]] constexpr size_type size() const noexcept { return size_; }
    [[nodiscard]] constexpr size_type capacity() const noexcept { return capacity_; }

    [[nodiscard]] constexpr value_type& front() noexcept { assert(!empty()); return data_[head]; }
    [[nodiscard]] constexpr value_type& back() noexcept { assert(!empty()); return data_[get_back_index()]; }
    [[nodiscard]] constexpr const value_type& front() const noexcept { assert(!empty()); return data_[head]; }
    [[nodiscard]] constexpr const value_type& back() const noexcept { assert(!empty()); return data_[get_back_index()]; }

    [[nodiscard]] constexpr size_type get_front_index() const noexcept { return head; }
    [[nodiscard]] constexpr size_type get_back_index() const noexcept { return (tail == 0 ? capacity_ : tail) - 1; }

    constexpr bool try_pop_back() noexcept {
        if (empty()) return false;
        pop_back();
        return true;
    }

    constexpr bool try_pop_front() noexcept {
        if (empty()) return false;
        pop_front();
        return true;
    }

    [[nodiscard]] constexpr value_type retrieve_front() noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        assert(!empty());
        value_type ret = std::move(front());
        pop_front();
        return ret;
    }

    [[nodiscard]] constexpr value_type retrieve_back() noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        assert(!empty());
        value_type ret = std::move(back());
        pop_back();
        return ret;
    }

    [[nodiscard]] constexpr std::optional<value_type> try_retrieve_front() noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        if (empty()) return std::nullopt;
        value_type ret = std::move(front());
        pop_front();
        return std::optional<value_type>{std::move(ret)};
    }

    [[nodiscard]] constexpr std::optional<value_type> try_retrieve_back() noexcept(std::is_nothrow_move_assignable_v<value_type>) {
        if (empty()) return std::nullopt;
        value_type ret = std::move(back());
        pop_back();
        return std::optional<value_type>{std::move(ret)};
    }

    FORCE_INLINE value_type& operator[](size_type index) noexcept {
        assert(index < size());
        assert(index < capacity());
        size_type actual_idx = index + head;
        if (actual_idx >= capacity_) actual_idx -= capacity_;
        return data_[actual_idx];
    }

    FORCE_INLINE const value_type& operator[](size_type index) const noexcept {
        assert(index < size());
        assert(index < capacity());
        size_type actual_idx = index + head;
        if (actual_idx >= capacity_) actual_idx -= capacity_;
        return data_[actual_idx];
    }

    value_type* data_at(size_type index) noexcept {
        size_type actual_idx = index + head;
        if (actual_idx >= capacity_) actual_idx -= capacity_;
        return std::to_address(data_) + actual_idx;
    }

    const value_type* data_at(size_type index) const noexcept {
        size_type actual_idx = index + head;
        if (actual_idx >= capacity_) actual_idx -= capacity_;
        return std::to_address(data_) + actual_idx;
    }

	template <std::invocable<size_type, value_type&> Fn>
		constexpr void for_each(Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, size_type, value_type&>){
    	size_type count{};
    	const size_type first_part_len = std::min(size_, capacity_ - head);
    	for(size_type i = 0; i < first_part_len; ++i){
    		std::invoke(fn, count++, data_[head + i]);
    	}

    	const size_type second_part_len = size_ - first_part_len;
    	for(size_type i = 0; i < second_part_len; ++i){
    		std::invoke(fn, count++, data_[i]);
    	}
    }

	template <std::invocable<size_type, const value_type&> Fn>
	constexpr void for_each(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, size_type, const value_type&>){
    	size_type count{};
    	const size_type first_part_len = std::min(size_, capacity_ - head);
    	for(size_type i = 0; i < first_part_len; ++i){
    		std::invoke(fn, count++, data_[head + i]);
    	}

    	const size_type second_part_len = size_ - first_part_len;
    	for(size_type i = 0; i < second_part_len; ++i){
    		std::invoke(fn, count++, data_[i]);
    	}
    }

private:
    ADAPTED_NO_UNIQUE_ADDRESS
    allocator_type alloc{};

    pointer data_{nullptr};
    size_type head{};
    size_type tail{};
    size_type size_{};
    size_type capacity_{};

    // --- 内存管理与异常安全 Helper ---

    constexpr void alloc_copy(auto first, auto last, auto d_first) {
        auto current = d_first;
        try {
            for (; first != last; ++first, ++current) {
                std::allocator_traits<allocator_type>::construct(alloc, std::to_address(current), *first);
            }
        } catch (...) {
            this->alloc_destroy(d_first, current);
            throw;
        }
    }

    constexpr void alloc_move(auto first, auto last, auto d_first) {
        auto current = d_first;
        try {
            for (; first != last; ++first, ++current) {
                std::allocator_traits<allocator_type>::construct(alloc, std::to_address(current), std::move(*first));
            }
        } catch (...) {
            this->alloc_destroy(d_first, current);
            throw;
        }
    }

    constexpr void alloc_destroy(auto first, auto last) noexcept {
        for (; first != last; ++first) {
            std::allocator_traits<allocator_type>::destroy(alloc, std::to_address(first));
        }
    }

    constexpr void destruct() noexcept {
        if constexpr (!std::is_trivially_destructible_v<value_type>) {
            if (size_ == 0) return;
            size_type first_part_len = std::min(size_, capacity_ - head);
            this->alloc_destroy(std::to_address(data_) + head, std::to_address(data_) + head + first_part_len);
            if (size_ > first_part_len) {
                this->alloc_destroy(std::to_address(data_), std::to_address(data_) + (size_ - first_part_len));
            }
        }
    }

    constexpr void incrTail() noexcept {
        assert(!full());
        ++size_;
        ++tail;
        if (tail == capacity_) {
            tail = 0;
        }
    }

    constexpr void tryExpand() {
        if constexpr (auto_resize) {
            if (full()) this->reserve(std::max<size_type>(capacity_ * 2, 8));
        } else {
            assert(!full());
        }
    }

    constexpr void decrHead() noexcept {
        assert(!full());
        ++size_;
        if (head == 0) {
            head = capacity_;
        }
        --head;
    }

public:
    friend constexpr void swap(circular_queue& lhs, circular_queue& rhs) noexcept(
        std::allocator_traits<allocator_type>::propagate_on_container_swap::value ||
        std::allocator_traits<allocator_type>::is_always_equal::value)
    {
        if constexpr (std::allocator_traits<allocator_type>::propagate_on_container_swap::value) {
            std::ranges::swap(lhs.alloc, rhs.alloc);
        } else {
            assert(lhs.alloc == rhs.alloc); // 按照标准，如果不可传播，交换不同分配器的容器是未定义行为
        }
        std::ranges::swap(lhs.data_, rhs.data_);
        std::ranges::swap(lhs.head, rhs.head);
        std::ranges::swap(lhs.tail, rhs.tail);
        std::ranges::swap(lhs.size_, rhs.size_);
        std::ranges::swap(lhs.capacity_, rhs.capacity_);
    }
};

} // namespace mo_yanxi
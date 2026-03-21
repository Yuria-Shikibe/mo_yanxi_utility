module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.circular_queue;

import std;

namespace mo_yanxi{
	export
	template <typename T, bool autoResize = true, std::unsigned_integral SizeType = std::size_t>
	struct circular_queue{
		static constexpr bool auto_resize = autoResize;
		using size_type = SizeType;
		using value_type = T;
		using allocator_type = std::allocator<value_type>;

		[[nodiscard]] constexpr circular_queue(){
			this->reserve(8);
		}

		[[nodiscard]] explicit constexpr circular_queue(const size_type length){
			this->reserve(length);
		}

		// 修复：使用 std::ranges::uninitialized_copy 进行安全拷贝，并直接对齐到 head = 0
		[[nodiscard]] constexpr circular_queue(const circular_queue& other, const size_type length)
			: data_{std::allocator_traits<allocator_type>::allocate(alloc, length)},
			  head{0},
			  tail{(other.size_ == length) ? 0 : other.size_},
			  size_{other.size_},
			  capacity_{length} {

			size_type first_part_len = std::min(other.size_, other.capacity_ - other.head);
			std::ranges::uninitialized_copy(
				other.data_ + other.head,
				other.data_ + other.head + first_part_len,
				data_,
				data_ + first_part_len
			);

			if (other.size_ > first_part_len) {
				std::ranges::uninitialized_copy(
					other.data_,
					other.data_ + (other.size_ - first_part_len),
					data_ + first_part_len,
					data_ + other.size_
				);
			}
		}

		constexpr circular_queue(const circular_queue& other)
			: circular_queue(other, other.capacity_) {}

		constexpr circular_queue(circular_queue&& other) noexcept :
			data_{std::exchange(other.data_, nullptr)},
			head{std::exchange(other.head, 0)},
			tail{std::exchange(other.tail, 0)},
			size_{std::exchange(other.size_, 0)},
			capacity_{std::exchange(other.capacity_, 0)} {
		}

		// 修复：使用 Copy-and-Swap 惯用法，保证强异常安全性
		constexpr circular_queue& operator=(const circular_queue& other){
			if(this == &other) return *this;
			circular_queue temp(other);
			std::swap(data_, temp.data_);
			std::swap(head, temp.head);
			std::swap(tail, temp.tail);
			std::swap(size_, temp.size_);
			std::swap(capacity_, temp.capacity_);
			return *this;
		}

		constexpr circular_queue& operator=(circular_queue&& other) noexcept{
			if(this == &other) return *this;
			std::swap(data_, other.data_);
			std::swap(head, other.head);
			std::swap(tail, other.tail);
			std::swap(size_, other.size_);
			std::swap(capacity_, other.capacity_);
			return *this;
		}

		constexpr ~circular_queue(){
			destruct();
			if(data_){
				std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
			}
		}

		constexpr void push_back(const value_type& val) noexcept(std::is_nothrow_copy_assignable_v<value_type>){
			tryExpand();
			std::construct_at(data_ + tail, val);
			incrTail();
		}

		constexpr void push_back(value_type&& val) noexcept(std::is_nothrow_move_assignable_v<value_type>){
			tryExpand();
			std::construct_at(data_ + tail, std::move(val));
			incrTail();
		}

		constexpr void push_front(const value_type& val) noexcept(std::is_nothrow_copy_assignable_v<value_type>){
			tryExpand();
			decrHead();
			std::construct_at(data_ + head, val);
		}

		constexpr void push_front(value_type&& val) noexcept(std::is_nothrow_move_assignable_v<value_type>){
			tryExpand();
			decrHead();
			std::construct_at(data_ + head, std::move(val));
		}

		template <typename... Args>
			requires (std::constructible_from<value_type, Args...>)
		constexpr value_type& emplace_front(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>){
			tryExpand();
			decrHead();
			std::construct_at(data_ + head, std::forward<Args>(args)...);
			return data_[head];
		}

		template <typename... Args>
			requires (std::constructible_from<value_type, Args...>)
		constexpr value_type& emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<value_type, Args...>){
			tryExpand();
			std::construct_at(data_ + tail, std::forward<Args>(args)...);
			value_type& rst = data_[tail];
			incrTail();
			return rst;
		}

		// 修复：更名为 reserve (符合 STL 语义)，并重构内存分配逻辑，避免野指针和 UB
		constexpr void reserve(const size_type newCapacity){
			if(newCapacity <= capacity_) return; // 环形队列通常不轻易缩容，若需缩容应确保 newCapacity >= size_

			auto newData = std::allocator_traits<allocator_type>::allocate(alloc, newCapacity);

			if(!empty()){
				size_type first_part_len = std::min(size_, capacity_ - head);
				std::ranges::uninitialized_move(
					data_ + head,
					data_ + head + first_part_len,
					newData,
					newData + first_part_len
				);

				if (size_ > first_part_len) {
					std::ranges::uninitialized_move(
						data_,
						data_ + (size_ - first_part_len),
						newData + first_part_len,
						newData + size_
					);
				}

				destruct();
				std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
			} else if (data_) {
				std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
			}

			data_ = newData;
			capacity_ = newCapacity;
			head = 0;
			// 修复：确保满载扩容时 tail 能够正确 wrap 到 0
			tail = (size_ == capacity_) ? 0 : size_;
		}

	private:
		ADAPTED_NO_UNIQUE_ADDRESS
		allocator_type alloc{};

		value_type* data_{nullptr};

		size_type head{};
		size_type tail{};
		size_type size_{};
		size_type capacity_{};

		// 修复：更安全的 destruct 逻辑，不依赖可能产生二义性的 isReversed()
		constexpr void destruct() const noexcept{
			if constexpr(!std::is_trivially_destructible_v<value_type>){
				if (size_ == 0) return;
				size_type first_part_len = std::min(size_, capacity_ - head);
				std::ranges::destroy(data_ + head, data_ + head + first_part_len);

				if (size_ > first_part_len) {
					std::ranges::destroy(data_, data_ + (size_ - first_part_len));
				}
			}
		}

		constexpr void incrTail() noexcept{
			assert(!full());
			++size_;
			++tail;
			if(tail == capacity_){
				tail = 0;
			}
		}

		constexpr void tryExpand(){
			if constexpr(auto_resize){
				if(full()) this->reserve(std::max<size_type>(capacity_ * 2, 8));
			} else{
				assert(!full());
			}
		}

		constexpr void decrHead() noexcept{
			assert(!full());
			++size_;
			if(head == 0){
				head = capacity_;
			}
			--head;
		}

	public:
		[[nodiscard]] constexpr bool empty() const noexcept{
			return size_ == 0;
		}

		[[nodiscard]] constexpr bool full() const noexcept{
			return size_ == capacity_;
		}

		[[nodiscard]] constexpr size_type size() const noexcept{
			return size_;
		}

		[[nodiscard]] constexpr size_type capacity() const noexcept{
			return capacity_;
		}

		constexpr void clear() noexcept{
			destruct();
			head = 0;
			tail = 0;
			size_ = 0;
		}

		[[nodiscard]] constexpr value_type& front() noexcept{
			assert(!empty());
			return data_[head];
		}

		[[nodiscard]] constexpr value_type& back() noexcept{
			assert(!empty());
			return data_[get_back_index()];
		}

		[[nodiscard]] constexpr const value_type& front() const noexcept{
			assert(!empty());
			return data_[head];
		}

		[[nodiscard]] constexpr const value_type& back() const noexcept{
			assert(!empty());
			return data_[get_back_index()];
		}

		[[nodiscard]] constexpr value_type front_or(const value_type& def) const noexcept requires (std::is_trivial_v<value_type>){
			if(empty())return def;
			return data_[head];
		}

		[[nodiscard]] constexpr value_type back_or(const value_type& def) const noexcept requires (std::is_trivial_v<value_type>){
			if(empty())return def;
			return data_[get_back_index()];
		}

		[[nodiscard]] constexpr size_type get_front_index() const noexcept{
			return head;
		}

		[[nodiscard]] constexpr size_type get_back_index() const noexcept{
			return (tail == 0 ? capacity_ : tail) - 1;
		}

		constexpr void pop_back() noexcept{
			assert(!empty());
			--size_;
			tail = get_back_index();
			if constexpr(!std::is_trivially_destructible_v<T>){
				std::destroy_at(data_ + tail);
			}
		}

		constexpr void pop_front() noexcept{
			assert(!empty());
			--size_;
			if constexpr(!std::is_trivially_destructible_v<T>){
				std::destroy_at(data_ + head);
			}
			++head;
			if(head == capacity_){
				head = 0;
			}
		}

		constexpr bool try_pop_back() noexcept{
			if(empty()) return false;
			pop_back();
			return true;
		}

		constexpr bool try_pop_front() noexcept{
			if(empty()) return false;
			pop_front();
			return true;
		}

		[[nodiscard]] constexpr value_type retrieve_front() noexcept(std::is_nothrow_move_assignable_v<value_type>){
			assert(!empty());
			value_type ret = std::move(front());
			pop_front();
			return ret;
		}

		[[nodiscard]] constexpr value_type retrieve_back() noexcept(std::is_nothrow_move_assignable_v<value_type>){
			assert(!empty());
			value_type ret = std::move(back());
			pop_back();
			return ret;
		}

		[[nodiscard]] constexpr std::optional<value_type> try_retrieve_front() noexcept(std::is_nothrow_move_assignable_v<value_type>){
			if(empty()) return std::nullopt;
			value_type ret = std::move(front());
			pop_front();
			return std::optional<value_type>{std::move(ret)};
		}

		[[nodiscard]] constexpr std::optional<value_type> try_retrieve_back() noexcept(std::is_nothrow_move_assignable_v<value_type>){
			if(empty()) return std::nullopt;
			value_type ret = std::move(back());
			pop_back();
			return std::optional<value_type>{std::move(ret)};
		}

		// 优化：消除 % 运算，转换为更高效的加减法
		FORCE_INLINE value_type& operator[](size_type index) noexcept{
			assert(index < size());
			assert(index < capacity());
			size_type actual_idx = index + head;
			if (actual_idx >= capacity_) actual_idx -= capacity_;
			return data_[actual_idx];
		}

		FORCE_INLINE const value_type& operator[](size_type index) const noexcept{
			assert(index < size());
			assert(index < capacity());
			size_type actual_idx = index + head;
			if (actual_idx >= capacity_) actual_idx -= capacity_;
			return data_[actual_idx];
		}

		// 优化：补充非 const 版本的 data_at，并消除 % 运算
		value_type* data_at(size_type index) noexcept{
			size_type actual_idx = index + head;
			if (actual_idx >= capacity_) actual_idx -= capacity_;
			return data_ + actual_idx;
		}

		const value_type* data_at(size_type index) const noexcept{
			size_type actual_idx = index + head;
			if (actual_idx >= capacity_) actual_idx -= capacity_;
			return data_ + actual_idx;
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

	public:
		friend constexpr void swap(circular_queue& lhs, circular_queue& rhs) noexcept {
			std::swap(lhs.data_, rhs.data_);
			std::swap(lhs.head, rhs.head);
			std::swap(lhs.tail, rhs.tail);
			std::swap(lhs.size_, rhs.size_);
			std::swap(lhs.capacity_, rhs.capacity_);
		}
	};
}
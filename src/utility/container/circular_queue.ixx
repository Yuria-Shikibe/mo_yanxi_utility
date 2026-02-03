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
			this->resize(8);
		}

		[[nodiscard]] explicit constexpr circular_queue(const size_type length){
			this->resize(length);
		}

		[[nodiscard]] constexpr circular_queue(const circular_queue& other, const size_type length){
			this->resize(length);
			other.copyTo(data_, capacity_);
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

		constexpr void resize(const size_type newCapacity){
			if(newCapacity == capacity_) return;
			auto newData = std::allocator_traits<allocator_type>::allocate(alloc, newCapacity);

			if(!empty()){
				assert(data_ != nullptr);

				if(newCapacity > capacity_){
					this->moveTo(newData, newCapacity);
					destruct();
				} else{
					// size_type currentIdx{};
					this->each([newCapacity, newData](const size_type idx, value_type& val){
						if(idx >= newCapacity) return;
						std::construct_at(newData + idx, std::move(val));
					});

					size_ = newCapacity;
				}

				std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
			}

			data_ = newData;
			capacity_ = newCapacity;
			head = 0;
			tail = size_;
		}

		constexpr ~circular_queue(){
			destruct();
			if(data_){
				std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
			}
		}

		constexpr circular_queue(const circular_queue& other)
			:
			data_{std::allocator_traits<allocator_type>::allocate(alloc, other.capacity_)},
			tail{other.size_},
			size_{other.size_},
			capacity_(other.capacity_){
			other.copyTo(data_, capacity_);
		}

		constexpr circular_queue(circular_queue&& other) noexcept :
			data_{std::exchange(other.data_, {})},
			head{std::exchange(other.head, {})},
			tail{std::exchange(other.tail, {})},
			size_{std::exchange(other.size_, {})},
			capacity_{std::exchange(other.capacity_, {})}{
		}

		constexpr circular_queue& operator=(const circular_queue& other){
			if(this == &other) return *this;
			destruct();
			this->resetHeadTail(0, other.size_, other.size_);
			if(capacity_ < other.capacity_){
				this->deallocAndReset(std::allocator_traits<allocator_type>::allocate(alloc, other.capacity_), other.capacity_);
				other.copyTo(data_, capacity_);
			} else{
				other.copyTo(data_, capacity_);
			}

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

	private:
		ADAPTED_NO_UNIQUE_ADDRESS
		allocator_type alloc{};

		value_type* data_{};

		size_type head{};
		size_type tail{};
		size_type size_{};
		size_type capacity_{};


		[[nodiscard]] constexpr bool isReversed() const noexcept{
			return head > tail || (head == tail && size_ == capacity_);
		}

		constexpr void resetHeadTail(const size_type head, const size_type tail, const size_type size) noexcept{
			this->head = head;
			this->tail = tail;
			this->size_ = size;
		}

		constexpr void deallocAndReset(value_type* ptr, const size_type newCapacity) noexcept{
			if(data_){
				std::allocator_traits<allocator_type>::deallocate(alloc, data_, capacity_);
				data_ = ptr;
				capacity_ = newCapacity;
			}
		}

		constexpr void destruct() const noexcept{
			if constexpr(!std::is_trivially_destructible_v<value_type>){
				if(isReversed()){
					std::ranges::destroy(data_ + head, data_ + capacity_);
					std::ranges::destroy(data_, data_ + tail);
				} else{
					std::ranges::destroy(data_ + head, data_ + tail);
				}
			}
		}

		constexpr void moveTo(value_type* dest, const size_type dest_Cap) const noexcept(std::is_nothrow_move_constructible_v<value_type>){
			size_type first_part_len = std::min(size_, capacity_ - head);
			std::ranges::uninitialized_move(data_ + head, data_ + head + first_part_len, dest, dest + dest_Cap);

			size_type second_part_len = size_ - first_part_len;
			if (second_part_len > 0) {
				std::ranges::uninitialized_move(data_, data_ + second_part_len, dest + first_part_len, dest + dest_Cap);
			}
		}


		constexpr void copyTo(value_type* dest, const size_type dest_Cap) const noexcept(std::is_nothrow_copy_constructible_v<value_type>){
			size_type first_part_len = std::min(size_, capacity_ - head);
			std::ranges::uninitialized_copy(data_ + head, data_ + head + first_part_len, dest, dest + dest_Cap);

			size_type second_part_len = size_ - first_part_len;
			if (second_part_len > 0) {
				std::ranges::uninitialized_copy(data_, data_ + second_part_len, dest + first_part_len, dest + dest_Cap);
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
				if(full()) this->resize(std::max<size_type>(capacity() * 2, 8));
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
			resetHeadTail(0, 0, 0);
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
			if(empty()){
				return false;
			}

			pop_back();
			return true;
		}

		constexpr bool try_pop_front() noexcept{
			if(empty()){
				return false;
			}

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
			if(empty()){
				return std::nullopt;
			}
			value_type ret = std::move(front());
			pop_front();
			return {std::move(ret)};
		}

		[[nodiscard]] constexpr std::optional<value_type> try_retrieve_back() noexcept(std::is_nothrow_move_assignable_v<value_type>){
			if(empty()){
				return std::nullopt;
			}
			value_type ret = std::move(back());
			pop_back();
			return {std::move(ret)};
		}

		FORCE_INLINE value_type& operator[](size_type index) noexcept{
			assert(index < size());

			assert(index < capacity());
			return data_[(index + head) % capacity_];
		}

		FORCE_INLINE const value_type& operator[](size_type index) const noexcept{
			assert(index < size());

			assert(index < capacity());
			return data_[(index + head) % capacity_];
		}

		const value_type* data_at(size_type index) const noexcept{
			return data_ + (index + head) % capacity_;
		}

		template <std::invocable<size_type, value_type&> Fn>
		constexpr void each(Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, size_type, value_type&>){
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
		constexpr void each(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, size_type, const value_type&>){
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
	};
}

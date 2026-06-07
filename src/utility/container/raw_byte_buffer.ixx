module;

#include <cassert>
#include <mo_yanxi/adapted_attributes.hpp>
#include <version>

export module mo_yanxi.raw_byte_buffer;

import std;

namespace mo_yanxi{
export template <typename T>
inline constexpr bool is_implicit_lifetime_v =
#ifdef __cpp_lib_is_implicit_lifetime
#ifdef __RESHARPER__
	true;
#else
std::is_implicit_lifetime_v<T>;
#endif

#else
std::is_trivial_v<T>;
#endif

export template <typename T>
concept implicit_lifetime_type =
	std::is_object_v<T> &&
	!std::is_const_v<T> &&
	!std::is_volatile_v<T> &&
	is_implicit_lifetime_v<T>;

export struct preserve_relocation_t{
	template <typename T, std::integral SizeTy>
		requires std::is_trivially_copyable_v<T>
	FORCE_INLINE static void operator()(const T* old_data, T* new_data, SizeTy count) noexcept{
		assert(old_data != nullptr);
		assert(new_data != nullptr);
		
		std::memcpy(
			static_cast<void*>(new_data),
			static_cast<const void*>(old_data),
			static_cast<std::size_t>(count) * sizeof(T));
	}
};

export inline constexpr preserve_relocation_t preserve_relocation{};

export struct discard_relocation_t{
	template <typename T, std::integral SizeTy>
	FORCE_INLINE static void operator()(const T*, T*, SizeTy) noexcept{
	}
};

export inline constexpr discard_relocation_t discard_relocation{};

export template <typename SizeTy>
concept raw_byte_buffer_size_type = std::unsigned_integral<SizeTy>;

export template <typename Relocator, typename T, typename SizeTy = std::size_t>
concept raw_byte_buffer_relocator =
	raw_byte_buffer_size_type<SizeTy> &&
	std::is_invocable_v<Relocator&, T*, T*, SizeTy>;

export template <typename Operation, typename T, typename SizeTy = std::size_t>
concept raw_byte_buffer_overwrite_operation =
	raw_byte_buffer_size_type<SizeTy> &&
	requires(Operation& operation, T* data, SizeTy old_size, SizeTy requested_size){
		{ std::invoke(operation, data, old_size, requested_size) } noexcept -> std::convertible_to<SizeTy>;
	} &&
	!std::signed_integral<std::remove_cvref_t<std::invoke_result_t<Operation&, T*, SizeTy, SizeTy>>>;

export
template <
	implicit_lifetime_type T = std::byte,
	typename Allocator = std::allocator<T>,
	raw_byte_buffer_size_type SizeTy = unsigned>
class raw_buffer{
public:
	using value_type = T;
	using allocator_type = Allocator;
	using allocator_traits = std::allocator_traits<allocator_type>;
	using propagate_on_container_copy_assignment = allocator_traits::propagate_on_container_copy_assignment;
	using propagate_on_container_move_assignment = allocator_traits::propagate_on_container_move_assignment;
	using propagate_on_container_swap = allocator_traits::propagate_on_container_swap;
	using is_always_equal = allocator_traits::is_always_equal;
	using size_type = SizeTy;
	using difference_type = std::ptrdiff_t;
	using pointer = allocator_traits::pointer;
	using const_pointer = allocator_traits::const_pointer;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = value_type*;
	using const_iterator = const value_type*;

	static_assert(std::same_as<typename allocator_traits::value_type, value_type>,
	              "raw_byte_buffer allocator value_type must match T");

private:
	ADAPTED_NO_UNIQUE_ADDRESS allocator_type allocator_{};
	pointer allocation_{};
	size_type size_{};
	size_type capacity_{};

public:
	[[nodiscard]] raw_buffer() noexcept(std::is_nothrow_default_constructible_v<allocator_type>) = default;

	[[nodiscard]] explicit raw_buffer(const allocator_type& allocator) noexcept(
		std::is_nothrow_copy_constructible_v<allocator_type>)
		: allocator_(allocator){
	}

	[[nodiscard]] raw_buffer(const raw_buffer& other) requires std::is_trivially_copyable_v<value_type>
		: allocator_(allocator_traits::select_on_container_copy_construction(other.allocator_)){
		copy_allocate_(other.data(), other.size_);
	}

	raw_buffer(const raw_buffer&) requires(!std::is_trivially_copyable_v<value_type>) = delete;

	raw_buffer& operator=(const raw_buffer& other) requires std::is_trivially_copyable_v<value_type>{
		if(this == std::addressof(other)) return *this;

		allocator_type target_allocator = allocator_;
		if constexpr(propagate_on_container_copy_assignment::value){
			target_allocator = other.allocator_;
		}

		raw_buffer temp{target_allocator};
		temp.copy_allocate_(other.data(), other.size_);

		release();
		if constexpr(propagate_on_container_copy_assignment::value){
			allocator_ = std::move(temp.allocator_);
		}
		allocation_ = std::exchange(temp.allocation_, pointer{});
		size_ = std::exchange(temp.size_, 0);
		capacity_ = std::exchange(temp.capacity_, 0);
		return *this;
	}

	raw_buffer& operator=(const raw_buffer&) requires(!std::is_trivially_copyable_v<value_type>) = delete;

	[[nodiscard]] raw_buffer(
		raw_buffer&& other) noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
		: allocator_(std::move(other.allocator_)),
		  allocation_(std::exchange(other.allocation_, pointer{})),
		  size_(std::exchange(other.size_, 0)),
		  capacity_(std::exchange(other.capacity_, 0)){
	}

	raw_buffer& operator=(raw_buffer&& other) noexcept(
		(propagate_on_container_move_assignment::value && std::is_nothrow_move_assignable_v<allocator_type>) ||
		is_always_equal::value){
		if(this == std::addressof(other)) return *this;

		if constexpr(!propagate_on_container_move_assignment::value && !is_always_equal::value){
			if(!(allocator_ == other.allocator_)){
				throw std::invalid_argument{"raw_byte_buffer move assignment requires equal allocators"};
			}
		}

		release();
		if constexpr(propagate_on_container_move_assignment::value){
			allocator_ = std::move(other.allocator_);
		}
		allocation_ = std::exchange(other.allocation_, pointer{});
		size_ = std::exchange(other.size_, 0);
		capacity_ = std::exchange(other.capacity_, 0);
		return *this;
	}

	~raw_buffer(){
		release();
	}

	[[nodiscard]] FORCE_INLINE value_type* data() noexcept{
		return capacity_ == 0 ? nullptr : std::to_address(allocation_);
	}

	[[nodiscard]] FORCE_INLINE const value_type* data() const noexcept{
		return capacity_ == 0 ? nullptr : std::to_address(allocation_);
	}

	[[nodiscard]] FORCE_INLINE iterator begin() noexcept{ return data(); }
	[[nodiscard]] FORCE_INLINE const_iterator begin() const noexcept{ return data(); }
	[[nodiscard]] FORCE_INLINE const_iterator cbegin() const noexcept{ return data(); }

	[[nodiscard]] FORCE_INLINE iterator end() noexcept{
		value_type* const first = data();
		return first == nullptr ? nullptr : first + size_;
	}

	[[nodiscard]] FORCE_INLINE const_iterator end() const noexcept{
		const value_type* const first = data();
		return first == nullptr ? nullptr : first + size_;
	}

	[[nodiscard]] FORCE_INLINE const_iterator cend() const noexcept{
		const value_type* const first = data();
		return first == nullptr ? nullptr : first + size_;
	}

	[[nodiscard]] FORCE_INLINE std::span<value_type> span() noexcept{
		return {data(), size_};
	}

	[[nodiscard]] FORCE_INLINE std::span<const value_type> span() const noexcept{
		return {data(), size_};
	}

	[[nodiscard]] FORCE_INLINE reference operator[](size_type index) noexcept{
		assert(index < size_);
		return data()[index];
	}

	[[nodiscard]] FORCE_INLINE const_reference operator[](size_type index) const noexcept{
		assert(index < size_);
		return data()[index];
	}

	[[nodiscard]] FORCE_INLINE reference front() noexcept{
		assert(!empty());
		return (*this)[0];
	}

	[[nodiscard]] FORCE_INLINE const_reference front() const noexcept{
		assert(!empty());
		return (*this)[0];
	}

	[[nodiscard]] FORCE_INLINE reference back() noexcept{
		assert(!empty());
		return (*this)[size_ - 1];
	}

	[[nodiscard]] FORCE_INLINE const_reference back() const noexcept{
		assert(!empty());
		return (*this)[size_ - 1];
	}

	[[nodiscard]] FORCE_INLINE size_type size() const noexcept{ return size_; }
	[[nodiscard]] FORCE_INLINE size_type capacity() const noexcept{ return capacity_; }
	[[nodiscard]] FORCE_INLINE bool empty() const noexcept{ return size_ == 0; }
	[[nodiscard]] FORCE_INLINE size_type size_bytes() const noexcept{ return size_ * sizeof(value_type); }
	[[nodiscard]] FORCE_INLINE size_type capacity_bytes() const noexcept{ return capacity_ * sizeof(value_type); }

	[[nodiscard]] allocator_type get_allocator() const noexcept(std::is_nothrow_copy_constructible_v<allocator_type>){
		return allocator_;
	}

	[[nodiscard]] size_type max_size() const noexcept{
		using allocator_size_type = typename allocator_traits::size_type;

		const allocator_size_type allocator_max = allocator_traits::max_size(allocator_);
		constexpr size_type size_type_max = std::numeric_limits<size_type>::max();

		if constexpr(std::numeric_limits<allocator_size_type>::digits <= std::numeric_limits<size_type>::digits){
			return static_cast<size_type>(allocator_max);
		} else{
			constexpr allocator_size_type converted_size_type_max = static_cast<allocator_size_type>(size_type_max);
			return allocator_max < converted_size_type_max ? static_cast<size_type>(allocator_max) : size_type_max;
		}
	}

	template <typename Relocator>
		requires raw_byte_buffer_relocator<Relocator, value_type, size_type>
	void reserve(size_type new_capacity, Relocator&& on_reallocate){
		// Contract: if reallocation happens, on_reallocate must establish the old [0, size) range in new storage.
		// If it throws, it must clean any destination objects it constructed before propagating.
		if(new_capacity <= capacity_) return;
		this->reallocate_(new_capacity, std::forward<Relocator>(on_reallocate));
	}

	void reserve(size_type new_capacity) requires std::is_trivially_copyable_v<value_type>{
		this->reserve(new_capacity, preserve_relocation);
	}

	template <typename Relocator>
		requires raw_byte_buffer_relocator<Relocator, value_type, size_type> && std::default_initializable<value_type>
	void resize(size_type new_size, Relocator&& on_reallocate){
		if(new_size < size_){
			this->destroy_n_(data() + new_size, size_ - new_size);
			size_ = new_size;
			return;
		}

		if(new_size == size_) return;

		const size_type old_size = size_;
		this->reserve(new_size, std::forward<Relocator>(on_reallocate));
		this->default_construct_n_(data() + old_size, new_size - old_size);
		size_ = new_size;
	}

	void resize(size_type new_size)
		requires(std::is_trivially_copyable_v<value_type> && std::default_initializable<value_type>){
		this->resize(new_size, preserve_relocation);
	}

	template <typename Relocator, typename Operation>
		requires raw_byte_buffer_relocator<Relocator, value_type, size_type> &&
		raw_byte_buffer_overwrite_operation<Operation, value_type, size_type> &&
		std::is_trivially_default_constructible_v<value_type> &&
		std::is_trivially_destructible_v<value_type>
	size_type resize_and_overwrite(size_type requested_size, Relocator&& on_reallocate, Operation&& operation){
		// Contract: operation must establish the final [0, returned_size) range.
		// Pass discard_relocation when old contents are intentionally ignored after a grow.
		const size_type old_size = size_;
		if(requested_size > capacity_){
			this->reallocate_(requested_size, std::forward<Relocator>(on_reallocate));
		}

		decltype(auto) returned_size = std::invoke(std::forward<Operation>(operation), data(), old_size, requested_size);
		const size_type final_size = checked_operation_size_(returned_size, requested_size);

		size_ = final_size;
		return final_size;
	}

	template <typename Operation>
		requires std::is_trivially_copyable_v<value_type> &&
		raw_byte_buffer_overwrite_operation<Operation, value_type, size_type> &&
		std::is_trivially_default_constructible_v<value_type> &&
		std::is_trivially_destructible_v<value_type>
	size_type resize_and_overwrite(size_type requested_size, Operation&& operation){
		return this->resize_and_overwrite(requested_size, preserve_relocation, std::forward<Operation>(operation));
	}

	void clear() noexcept{
		this->destroy_n_(data(), size_);
		size_ = 0;
	}

	void release() noexcept{
		clear();
		if(capacity_ != 0){
			allocator_traits::deallocate(allocator_, allocation_, capacity_);
			allocation_ = pointer{};
			capacity_ = 0;
		}
	}

	friend void swap(raw_buffer& lhs, raw_buffer& rhs) noexcept(
		propagate_on_container_swap::value || is_always_equal::value){
		if constexpr(propagate_on_container_swap::value){
			std::ranges::swap(lhs.allocator_, rhs.allocator_);
		} else if constexpr(!is_always_equal::value){
			assert(lhs.allocator_ == rhs.allocator_);
		}

		std::ranges::swap(lhs.allocation_, rhs.allocation_);
		std::ranges::swap(lhs.size_, rhs.size_);
		std::ranges::swap(lhs.capacity_, rhs.capacity_);
	}

protected:
	[[nodiscard]] FORCE_INLINE allocator_type& allocator_ref_() noexcept{
		return allocator_;
	}

	[[nodiscard]] FORCE_INLINE const allocator_type& allocator_ref_() const noexcept{
		return allocator_;
	}

	template <typename... Args>
	FORCE_INLINE void construct_at_(value_type* target, Args&&... args){
		allocator_traits::construct(allocator_, target, std::forward<Args>(args)...);
	}

	void check_capacity_(size_type new_capacity) const{
		if(new_capacity > max_size()){
			throw std::bad_array_new_length{};
		}
	}

	void set_size_(size_type new_size) noexcept{
		assert(new_size <= capacity_);
		size_ = new_size;
	}

	void copy_allocate_(const value_type* source, size_type count) requires std::is_trivially_copyable_v<value_type>{
		if(count == 0){
			return;
		}

		this->check_capacity_(count);
		allocation_ = allocator_traits::allocate(allocator_, count);
		capacity_ = count;
		std::memcpy(
			static_cast<void*>(std::to_address(allocation_)),
			static_cast<const void*>(source),
			static_cast<std::size_t>(count) * sizeof(value_type));
		size_ = count;
	}

	template <typename ReturnedSize>
	[[nodiscard]] static size_type checked_operation_size_(ReturnedSize&& returned_size, size_type requested_size) noexcept{
		if constexpr(std::integral<std::remove_cvref_t<ReturnedSize>>){
			if constexpr(std::signed_integral<std::remove_cvref_t<ReturnedSize>>){
				const bool returned_negative = returned_size < 0;
				assert(!returned_negative && "resize_and_overwrite returned a negative size");
				if(returned_negative) [[unlikely]] {
					std::terminate();
				}
			}

			const bool returned_too_large = std::cmp_greater(returned_size, requested_size);
			assert(!returned_too_large && "resize_and_overwrite returned a size larger than requested");
			if(returned_too_large) [[unlikely]] {
				std::terminate();
			}
		}

		const size_type final_size = static_cast<size_type>(std::forward<ReturnedSize>(returned_size));
		assert(final_size <= requested_size && "resize_and_overwrite returned a size larger than requested");
		if(final_size > requested_size) [[unlikely]] {
			std::terminate();
		}
		return final_size;
	}

	template <typename Relocator>
	void reallocate_(size_type new_capacity, Relocator&& on_reallocate){
		this->check_capacity_(new_capacity);

		pointer new_allocation = allocator_traits::allocate(allocator_, new_capacity);
		value_type* const old_data = data();
		value_type* const new_data = std::to_address(new_allocation);

		if(capacity_ != 0){
			if constexpr(std::is_nothrow_invocable_v<Relocator&, value_type*, value_type*, size_type>){
				std::invoke(std::forward<Relocator>(on_reallocate), old_data, new_data, size_);
			} else{
				try{
					std::invoke(std::forward<Relocator>(on_reallocate), old_data, new_data, size_);
				} catch(...){
					allocator_traits::deallocate(allocator_, new_allocation, new_capacity);
					throw;
				}
			}
			this->destroy_n_(old_data, size_);
			allocator_traits::deallocate(allocator_, allocation_, capacity_);
		}

		allocation_ = new_allocation;
		capacity_ = new_capacity;
	}

	void default_construct_n_(value_type* first, size_type count){
		if(count == 0) return;

		if constexpr(!std::is_trivially_default_constructible_v<value_type>){
			size_type constructed = 0;
			try{
				for(; constructed < count; ++constructed){
					allocator_traits::construct(allocator_, first + constructed);
				}
			} catch(...){
				this->destroy_n_(first, constructed);
				throw;
			}
		}
	}

	void destroy_n_(value_type* first, size_type count) noexcept{
		if constexpr(!std::is_trivially_destructible_v<value_type>){
			for(size_type idx = 0; idx < count; ++idx){
				allocator_traits::destroy(allocator_, first + idx);
			}
		}
	}
};

export template <
	implicit_lifetime_type T = std::byte,
	typename Allocator = std::allocator<T>,
	raw_byte_buffer_size_type SizeTy = unsigned>
class raw_vector : protected raw_buffer<T, Allocator, SizeTy>{
	using base_type = raw_buffer<T, Allocator, SizeTy>;
	using allocator_type_ = typename base_type::allocator_type;
	using allocator_traits_ = typename base_type::allocator_traits;
	using value_type_ = typename base_type::value_type;

	static constexpr bool allocator_move_constructible_ = requires(allocator_type_& allocator,
	                                                               value_type_* target,
	                                                               value_type_* source){
		allocator_traits_::construct(allocator, target, std::move(*source));
	};

	static consteval bool allocator_move_construct_is_nothrow_test_(){
		if constexpr(allocator_move_constructible_){
			return noexcept(allocator_traits_::construct(
				std::declval<allocator_type_&>(),
				std::declval<value_type_*>(),
				std::declval<value_type_&&>()));
		} else{
			return false;
		}
	}

	static constexpr bool allocator_move_construct_is_nothrow_ = allocator_move_construct_is_nothrow_test_();

public:
	using typename base_type::value_type;
	using typename base_type::allocator_type;
	using typename base_type::allocator_traits;
	using typename base_type::propagate_on_container_copy_assignment;
	using typename base_type::propagate_on_container_move_assignment;
	using typename base_type::propagate_on_container_swap;
	using typename base_type::is_always_equal;
	using typename base_type::size_type;
	using typename base_type::difference_type;
	using typename base_type::pointer;
	using typename base_type::const_pointer;
	using typename base_type::reference;
	using typename base_type::const_reference;
	using typename base_type::iterator;
	using typename base_type::const_iterator;

	static constexpr bool default_relocation_available =
		std::is_trivially_copyable_v<value_type> || allocator_move_constructible_;
	static constexpr bool default_relocation_is_nothrow =
		std::is_trivially_copyable_v<value_type> || allocator_move_construct_is_nothrow_;

	[[nodiscard]] raw_vector() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
	= default;

	[[nodiscard]] explicit raw_vector(const allocator_type& allocator) noexcept(
		std::is_nothrow_copy_constructible_v<allocator_type>)
		: base_type(allocator){
	}

	[[nodiscard]] raw_vector(const raw_vector& other) requires std::is_trivially_copyable_v<value_type>
	= default;

	raw_vector(const raw_vector&) requires(!std::is_trivially_copyable_v<value_type>) = delete;

	raw_vector& operator=(const raw_vector& other) requires std::is_trivially_copyable_v<value_type>
	= default;

	raw_vector& operator=(const raw_vector&) requires(!std::is_trivially_copyable_v<value_type>) = delete;

	[[nodiscard]] raw_vector(raw_vector&& other) noexcept(
		std::is_nothrow_move_constructible_v<base_type>) = default;

	raw_vector& operator=(raw_vector&& other) noexcept(
		std::is_nothrow_move_assignable_v<base_type>) = default;

	~raw_vector() = default;

	using base_type::begin;
	using base_type::capacity;
	using base_type::capacity_bytes;
	using base_type::cbegin;
	using base_type::cend;
	using base_type::clear;
	using base_type::data;
	using base_type::empty;
	using base_type::end;
	using base_type::front;
	using base_type::get_allocator;
	using base_type::max_size;
	using base_type::operator[];
	using base_type::back;
	using base_type::release;
	using base_type::size;
	using base_type::size_bytes;
	using base_type::span;

	void reserve(size_type new_capacity) requires default_relocation_available{
		base_type::reserve(new_capacity, default_relocator{this});
	}

	template <typename Relocator>
		requires raw_byte_buffer_relocator<Relocator, value_type, size_type>
	void reserve(size_type new_capacity, Relocator&& on_reallocate){
		base_type::reserve(new_capacity, std::forward<Relocator>(on_reallocate));
	}

	void resize(size_type new_size) requires(default_relocation_available && std::default_initializable<value_type>){
		base_type::resize(new_size, default_relocator{this});
	}

	template <typename Relocator>
		requires raw_byte_buffer_relocator<Relocator, value_type, size_type> && std::default_initializable<value_type>
	void resize(size_type new_size, Relocator&& on_reallocate){
		base_type::resize(new_size, std::forward<Relocator>(on_reallocate));
	}

	template <typename Operation>
		requires default_relocation_available &&
		raw_byte_buffer_overwrite_operation<Operation, value_type, size_type> &&
		std::is_trivially_default_constructible_v<value_type> &&
		std::is_trivially_destructible_v<value_type>
	size_type resize_and_overwrite(size_type requested_size, Operation&& operation){
		return base_type::resize_and_overwrite(requested_size, default_relocator{this},
		                                       std::forward<Operation>(operation));
	}

	template <typename Relocator, typename Operation>
		requires raw_byte_buffer_relocator<Relocator, value_type, size_type> &&
		raw_byte_buffer_overwrite_operation<Operation, value_type, size_type> &&
		std::is_trivially_default_constructible_v<value_type> &&
		std::is_trivially_destructible_v<value_type>
	size_type resize_and_overwrite(size_type requested_size, Relocator&& on_reallocate, Operation&& operation){
		return base_type::resize_and_overwrite(requested_size, std::forward<Relocator>(on_reallocate),
		                                       std::forward<Operation>(operation));
	}

	template <typename... Args>
		requires default_relocation_available && std::constructible_from<value_type, Args...>
	reference emplace_back(Args&&... args){
		const size_type old_size = this->size();
		if(old_size == this->capacity()){
			this->grow_for_append_(old_size + 1);
		}

		value_type* const target = this->data() + old_size;
		this->construct_at_(target, std::forward<Args>(args)...);
		this->set_size_(old_size + 1);
		return *target;
	}

	void push_back(const value_type& value)
		requires default_relocation_available && std::copy_constructible<value_type>{
		(void)this->emplace_back(value);
	}

	void push_back(value_type&& value)
		requires default_relocation_available && std::move_constructible<value_type>{
		(void)this->emplace_back(std::move(value));
	}

	void pop_back() noexcept{
		assert(!this->empty());
		const size_type old_size = this->size();
		this->destroy_n_(this->data() + old_size - 1, 1);
		this->set_size_(old_size - 1);
	}

	friend void swap(raw_vector& lhs, raw_vector& rhs) noexcept(std::is_nothrow_swappable_v<base_type>){
		std::ranges::swap(static_cast<base_type&>(lhs), static_cast<base_type&>(rhs));
	}

private:
	struct default_relocator{
		raw_vector* owner;

		FORCE_INLINE void operator()(value_type* old_data, value_type* new_data, size_type count) const noexcept(
			default_relocation_is_nothrow){
			owner->default_relocate_(old_data, new_data, count);
		}
	};

	void default_relocate_(value_type* old_data, value_type* new_data, size_type count) noexcept(
		default_relocation_is_nothrow) requires default_relocation_available{
		if(count == 0) return;

		if constexpr(std::is_trivially_copyable_v<value_type>){
			preserve_relocation(old_data, new_data, count);
		} else if constexpr(default_relocation_is_nothrow){
			for(size_type idx = 0; idx < count; ++idx){
				this->construct_at_(new_data + idx, std::move(old_data[idx]));
			}
		} else{
			size_type constructed = 0;
			try{
				for(; constructed < count; ++constructed){
					this->construct_at_(new_data + constructed, std::move(old_data[constructed]));
				}
			} catch(...){
				this->destroy_n_(new_data, constructed);
				throw;
			}
		}
	}

	void grow_for_append_(size_type required_capacity){
		size_type new_capacity = this->capacity() == 0 ? 1 : this->capacity() * 2;
		if(new_capacity < this->capacity()){
			throw std::bad_array_new_length{};
		}
		if(new_capacity < required_capacity){
			new_capacity = required_capacity;
		}
		this->check_capacity_(new_capacity);
		base_type::reserve(new_capacity, default_relocator{this});
	}
};
} // namespace mo_yanxi

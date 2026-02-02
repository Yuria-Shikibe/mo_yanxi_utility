module;

#include <cassert>

export module mo_yanxi.strided_span;

import mo_yanxi.meta_programming;
import std;

namespace mo_yanxi{

	template <class It, class T>
	concept Span_compatible_iterator =
		std::contiguous_iterator<It> && std::is_convertible_v<std::remove_reference_t<std::iter_reference_t<It>> (*)[], T (*)[]>;

	template <class Se, class It>
	concept Span_compatible_sentinel = std::sized_sentinel_for<Se, It> && !std::is_convertible_v<Se, size_t>;

	export
	enum span_stride : std::ptrdiff_t{
		dynamic_in_byte = 0//std::numeric_limits<std::ptrdiff_t>::max(),
	};

	template <typename T, T size>
	struct optional_dynamic_size{
		static constexpr T value = size;

		[[nodiscard]] optional_dynamic_size() = default;
		[[nodiscard]] optional_dynamic_size(T t){}

		constexpr explicit(false) operator T() const noexcept{
			return value;
		}
	};

	template <>
	struct optional_dynamic_size<std::size_t, std::dynamic_extent>{
		std::size_t value{};

		constexpr explicit(false) operator std::size_t() const noexcept{
			return value;
		}
	};

	template <>
	struct optional_dynamic_size<std::ptrdiff_t, dynamic_in_byte>{
		std::ptrdiff_t value{};

		constexpr explicit(false) operator std::ptrdiff_t() const noexcept{
			return value;
		}
	};

	export
	template <class Ty, std::ptrdiff_t Stride = dynamic_in_byte>
	struct strided_span_iterator {
		using iterator_concept  = std::contiguous_iterator_tag;
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = std::remove_cv_t<Ty>;
		using difference_type   = std::ptrdiff_t;
		using pointer           = Ty*;
		using reference         = Ty&;

		[[nodiscard]] constexpr reference operator*() const noexcept {
			assert(ptr != nullptr);
			return *ptr;
		}

		[[nodiscard]] constexpr pointer operator->() const noexcept {
			return ptr;
		}

		constexpr strided_span_iterator& operator++() noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) + stride.value);
			}else{
				++ptr;
			}

			return *this;
		}

		constexpr strided_span_iterator operator++(int) noexcept {
			strided_span_iterator tmp{*this};
			++*this;
			return tmp;
		}

		constexpr strided_span_iterator& operator--() noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) - stride.value);
			}else{
				--ptr;
			}
			return *this;
		}

		constexpr strided_span_iterator operator--(int) noexcept {
			strided_span_iterator tmp{*this};
			--*this;
			return tmp;
		}

		constexpr strided_span_iterator& operator+=(const difference_type off) noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) + off * stride.value);
			}else{
				ptr += off;
			}

			return *this;
		}

		[[nodiscard]] constexpr strided_span_iterator operator+(const difference_type Off) const noexcept {
			strided_span_iterator Tmp{*this};
			Tmp += Off;
			return Tmp;
		}

		[[nodiscard]] friend constexpr strided_span_iterator operator+(const difference_type Off, strided_span_iterator Next) noexcept {
			Next += Off;
			return Next;
		}

		constexpr strided_span_iterator& operator-=(const difference_type off) noexcept {
			if(stride.value){
				ptr = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr)) - off * stride.value);
			}else{
				ptr -= off;
			}

			return *this;
		}

		[[nodiscard]] constexpr strided_span_iterator operator-(const difference_type off) const noexcept {
			strided_span_iterator tmp{*this};
			tmp -= off;
			return tmp;
		}

		[[nodiscard]] constexpr difference_type operator-(const strided_span_iterator& right) const noexcept {
			if(stride.value){
				return (reinterpret_cast<const std::byte*>(ptr) - reinterpret_cast<const std::byte*>(right.ptr)) / stride.value;
			}else{
				return ptr - right.ptr;
			}
		}

		[[nodiscard]] constexpr reference operator[](const difference_type off) const noexcept {
			return *(*this + off);
		}

		[[nodiscard]] constexpr bool operator==(const strided_span_iterator& right) const noexcept {
			return ptr == right.ptr;
		}

		[[nodiscard]] constexpr auto operator<=>(const strided_span_iterator& right) const noexcept {
			return ptr <=> right.ptr;
		}

		[[nodiscard]] strided_span_iterator() = default;

		[[nodiscard]] explicit(false) strided_span_iterator(pointer ptr, std::ptrdiff_t stride = 0)
			: ptr(ptr),
			  stride(stride){
		}

	private:
		pointer ptr = nullptr;
		optional_dynamic_size<difference_type, Stride> stride{};

	};

	export
	template <class Ty, std::size_t Extent = std::dynamic_extent, std::ptrdiff_t Stride = dynamic_in_byte>
	class strided_span{
	private:
		Ty* ptr_;
		optional_dynamic_size<std::size_t, Extent> extent_;
		optional_dynamic_size<std::ptrdiff_t, Stride> stride_;

	public:
		using element_type = Ty;
		using value_type = std::remove_cv_t<Ty>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = Ty*;
		using const_pointer = const Ty*;
		using reference = Ty&;
		using const_reference = const Ty&;
		using iterator = strided_span_iterator<Ty>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_iterator         = std::const_iterator<iterator>;
		using const_reverse_iterator = std::const_iterator<reverse_iterator>;


		template <class Ty_, std::size_t Extent_, std::ptrdiff_t Stride_>
		friend class strided_span;
		// static constexpr size_type extent = Extent;

		// [span.cons] Constructors, copy, and assignment
		constexpr strided_span() noexcept requires (Extent == 0 || Extent == std::dynamic_extent)
		= default;

		template <Span_compatible_iterator<element_type> It>
		constexpr explicit(Extent != std::dynamic_extent) strided_span(It first, size_type count, difference_type stride = 0) noexcept // strengthened
			: ptr_(std::to_address(first)), extent_(count), stride_(stride){
		}

		constexpr explicit(Extent != std::dynamic_extent) strided_span(pointer first, size_type count, difference_type stride = 0) noexcept // strengthened
			: ptr_(first), extent_(count), stride_(stride){
		}


		template <Span_compatible_iterator<element_type> It, Span_compatible_sentinel<It> Se>
		constexpr explicit(Extent != std::dynamic_extent) strided_span(It first, Se last, difference_type stride = 0)
			noexcept(noexcept(last - first)) // strengthened
			: ptr_(std::to_address(first)), extent_(static_cast<size_type>(last - first)), stride_(stride){
		}

		template <typename T>
		constexpr explicit strided_span(strided_span<T, Extent, Stride> other)
			: ptr_(reinterpret_cast<Ty*>(other.data())), extent_(other.extent_.value * sizeof(T) / sizeof(value_type)), stride_(other.stride_.value){
		}


		strided_span(const strided_span& other) = default;
		strided_span(strided_span&& other) noexcept = default;
		strided_span& operator=(const strided_span& other) = default;
		strided_span& operator=(strided_span&& other) noexcept = default;

		constexpr explicit(false) operator strided_span<std::byte, Extent, Stride>() noexcept{
			return {reinterpret_cast<std::byte*>(data()), extent_  * sizeof(value_type), stride_};
		}

		constexpr explicit(false) operator strided_span<const std::byte, Extent, Stride>() const noexcept {
			return {reinterpret_cast<const std::byte*>(data()), extent_  * sizeof(value_type), stride_};
		}
		// [span.obs] Observers
		[[nodiscard]] constexpr size_type size() const noexcept{
			return extent_.value;
		}

		[[nodiscard]] constexpr difference_type stride() const noexcept{
			return stride_.value;
		}

		[[nodiscard]] constexpr size_type size_bytes() const noexcept{
			return extent_.value * sizeof(element_type);
		}

		[[nodiscard]] constexpr bool empty() const noexcept{
			return extent_.value == 0;
		}

		// [span.elem] Element access
		[[nodiscard]] constexpr reference operator[](const size_type off) const noexcept /* strengthened */{
			assert(ptr_);
			assert(off < extent_.value);

			if(!stride_.value){
				return ptr_[off];
			}else{
				const auto dst = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr_)) + off * stride_.value);
				return *dst;
			}
		}

		[[nodiscard]] constexpr reference front() const noexcept /* strengthened */{
			assert(ptr_);
			assert(extent_.value > 0);
			return *ptr_;
		}

		[[nodiscard]] constexpr reference back() const noexcept /* strengthened */{
			assert(ptr_);
			assert(extent_.value > 0);
			if(stride_.value){
				const auto end = ptr_ + extent_ - 1;
				return *end;
			}else{
				const auto end = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr_)) + (extent_ - 1) * stride_.value);
				return *end;
			}
		}

		[[nodiscard]] constexpr pointer data() const noexcept{
			return ptr_;
		}

		// [span.iterators] Iterator support
		[[nodiscard]] constexpr iterator begin() const noexcept{
			return iterator{ptr_, stride_.value};
		}

		[[nodiscard]] constexpr iterator end() const noexcept{
			if(stride_.value){
				const auto end = reinterpret_cast<pointer>(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(ptr_)) + (extent_.value * stride_.value));
				return iterator{end, stride_.value};
			}else{
				const auto end = ptr_ + extent_.value;
				return iterator{end};
			}
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
			return begin();
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept {
			return end();
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept{
			return reverse_iterator{end()};
		}

		[[nodiscard]] constexpr reverse_iterator rend() const noexcept{
			return reverse_iterator{begin()};
		}

		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
			return rbegin();
		}

		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
			return rend();
		}
	};

	template <typename Ty, typename Cond>
	using cond_add_const_to = std::conditional_t<std::is_const_v<Cond>, std::add_const_t<Ty>, Ty>;

	export
	template <class ValueTuple>
	struct strided_multi_span_iterator {
		using base_types = tuple_const_to_inner_t<ValueTuple>;
		using ptr_internal = std::byte*;

	public:
		using iterator_concept  = std::contiguous_iterator_tag;
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = unary_apply_to_tuple_t<std::add_lvalue_reference_t, tuple_const_to_inner_t<ValueTuple>>;
		using difference_type   = std::ptrdiff_t;
		using pointer           = value_type*;
		using reference         = value_type&;

		[[nodiscard]] constexpr value_type operator*() const noexcept {
			return [this] <std::size_t ...Idx>(std::index_sequence<Idx...>) {
				return value_type{reinterpret_cast<std::tuple_element_t<Idx, value_type>>(*(ptr + offsets_[Idx])) ...};
			}(std::make_index_sequence<std::tuple_size_v<value_type>>{});
		}

		constexpr strided_multi_span_iterator& operator++() noexcept {
			ptr += stride;

			return *this;
		}

		constexpr strided_multi_span_iterator operator++(int) noexcept {
			strided_multi_span_iterator tmp{*this};
			++*this;
			return tmp;
		}

		constexpr strided_multi_span_iterator& operator--() noexcept {
			ptr -= stride;

			return *this;
		}

		constexpr strided_multi_span_iterator operator--(int) noexcept {
			strided_multi_span_iterator tmp{*this};
			--*this;
			return tmp;
		}

		constexpr strided_multi_span_iterator& operator+=(const difference_type off) noexcept {
			ptr += off * stride;


			return *this;
		}

		[[nodiscard]] constexpr strided_multi_span_iterator operator+(const difference_type Off) const noexcept {
			strided_multi_span_iterator Tmp{*this};
			Tmp += Off;
			return Tmp;
		}

		[[nodiscard]] friend constexpr strided_multi_span_iterator operator+(const difference_type Off, strided_multi_span_iterator Next) noexcept {
			Next += Off;
			return Next;
		}

		constexpr strided_multi_span_iterator& operator-=(const difference_type off) noexcept {
			ptr -= off * stride;


			return *this;
		}

		[[nodiscard]] constexpr strided_multi_span_iterator operator-(const difference_type off) const noexcept {
			strided_multi_span_iterator tmp{*this};
			tmp -= off;
			return tmp;
		}

		[[nodiscard]] constexpr difference_type operator-(const strided_multi_span_iterator& right) const noexcept {
			return (ptr - right.ptr) / stride;
		}

		[[nodiscard]] constexpr reference operator[](const difference_type off) const noexcept {
			return *(*this + off);
		}

		[[nodiscard]] constexpr bool operator==(const strided_multi_span_iterator& right) const noexcept {
			return ptr == right.ptr;
		}

		[[nodiscard]] constexpr auto operator<=>(const strided_multi_span_iterator& right) const noexcept {
			return ptr <=> right.ptr;
		}

		[[nodiscard]] strided_multi_span_iterator() = default;

		[[nodiscard]] explicit(false) strided_multi_span_iterator(ptr_internal ptr) : ptr(ptr){}

		[[nodiscard]] explicit(false) strided_multi_span_iterator(
			ptr_internal ptr,
			std::array<std::ptrdiff_t, std::tuple_size_v<ValueTuple>> offs,
			std::ptrdiff_t stride)
			: ptr(ptr),
			  offsets_(offs),
			  stride(stride){
		}

	private:
		ptr_internal ptr = nullptr;
		std::array<std::ptrdiff_t, std::tuple_size_v<ValueTuple>> offsets_;
		std::ptrdiff_t stride;
	};

	export
	template <typename ValueTuple>
	struct strided_multi_span{
	private:
		static constexpr auto tuple_sz = std::tuple_size_v<ValueTuple>;
		static_assert(tuple_sz != 0);
		std::byte* data_;
		std::array<std::ptrdiff_t, std::tuple_size_v<ValueTuple>> offsets_;
		std::size_t size_;
		std::ptrdiff_t stride_;

	public:
		using value_type        = unary_apply_to_tuple_t<std::add_lvalue_reference_t, tuple_const_to_inner_t<ValueTuple>>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;
		using iterator = strided_multi_span_iterator<value_type>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_iterator         = strided_multi_span_iterator<const value_type>;
		using const_reverse_iterator = std::const_iterator<reverse_iterator>;

		[[nodiscard]] constexpr strided_multi_span() = default;

		template <typename ...Ty>
		[[nodiscard]] constexpr strided_multi_span(strided_span<Ty> ...spans) noexcept
			:
			data_(std::min(std::initializer_list<std::byte*>{
					const_cast<std::byte*>(reinterpret_cast<const std::byte*>(spans.data())) ...
				})),
			offsets_{
				(const_cast<std::byte*>(reinterpret_cast<const std::byte*>(spans.data())) - data_) ...
			},
			size_(std::min(std::initializer_list<std::size_t>{spans.size() ...})),
			stride_((spans.stride(), ...))
		{
			if(((stride_ != spans.stride()) || ...)){
				std::terminate();
			}


		}

		[[nodiscard]] constexpr strided_multi_span(const unary_apply_to_tuple_t<strided_span, tuple_const_to_inner_t<ValueTuple>>& spans) noexcept{
			[&, this] <std::size_t ...Idx>(std::index_sequence<Idx...>){
				this->operator=(strided_multi_span{std::get<Idx>(spans) ...});
			}(std::make_index_sequence<tuple_sz>{});
		}

		value_type operator[](std::size_t idx) const noexcept{
			return [&, this] <std::size_t ...Idx>(std::index_sequence<Idx...>) {
				return value_type{reinterpret_cast<std::tuple_element_t<Idx, value_type>>(*(data_ + idx * offsets_ + offsets_[Idx])) ...};
			}(std::make_index_sequence<std::tuple_size_v<value_type>>{});
		}

		[[nodiscard]] constexpr std::size_t size() const noexcept{
			return size_;
		}

		[[nodiscard]] iterator begin() const noexcept{
			return iterator{data_, offsets_, stride_};
		}

		[[nodiscard]] iterator end() const noexcept{
			return iterator{data_ + size_ * stride_};
		}

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
			return begin();
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept {
			return end();
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept{
			return reverse_iterator{end()};
		}

		[[nodiscard]] constexpr reverse_iterator rend() const noexcept{
			return reverse_iterator{begin()};
		}

		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
			return rbegin();
		}

		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
			return rend();
		}
	};
}

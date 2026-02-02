module;

#include "mo_yanxi/adapted_attributes.hpp"
#include <cassert>
#include <version>

export module mo_yanxi.dim2.tile;

import ext.dim2.plane_concept;
import mo_yanxi.math.vector2;
import std;

export namespace mo_yanxi::dim2{
	// template <typename Derived, typename Base>
	// concept remove_cvref_derived_from = std::derived_from<std::remove_cvref_t<Derived>, Base>;

	template <typename Impl, typename T, std::unsigned_integral SizeType = unsigned>
	struct tile_adaptor{
		using value_type = T;
		using size_type = SizeType;

		using packed_index_type = std::conditional_t<
			sizeof(size_type) == 1, std::uint16_t, std::conditional_t<
				sizeof(size_type) == 2, std::uint32_t, std::conditional_t<
					sizeof(size_type) == 4, std::uint64_t, void>>>;


	protected:
		template <typename Ty = value_type>
		struct tile_adaptor_iterator : std::contiguous_iterator_tag{
			using iterator_category = std::contiguous_iterator_tag;
			using value_type = Ty;
			using difference_type = std::ptrdiff_t;

			value_type* data{};

			[[nodiscard]] constexpr tile_adaptor_iterator() = default;

			[[nodiscard]] constexpr explicit(false) tile_adaptor_iterator(value_type* data)
				: data{data}{}

			constexpr friend bool operator==(const tile_adaptor_iterator& a, const tile_adaptor_iterator& b) noexcept{
				return a.data == b.data;
			}

			constexpr auto operator<=>(const tile_adaptor_iterator& o) const noexcept{
				return data <=> o.data;
			}

			constexpr value_type& operator*() noexcept{
				return *data;
			}

			constexpr value_type& operator*() const noexcept{
				return *data;
			}

			constexpr value_type* operator->() noexcept{
				return data;
			}

			constexpr value_type* operator->() const noexcept{
				return data;
			}

			constexpr tile_adaptor_iterator& operator++() noexcept{
				++data;
				return *this;
			}

			constexpr tile_adaptor_iterator operator++(int) noexcept{
				const tile_adaptor_iterator tmp = *this;
				++(*this);
				return tmp;
			}

			constexpr tile_adaptor_iterator& operator--() noexcept{
				--data;
				return *this;
			}

			constexpr tile_adaptor_iterator operator--(int) noexcept{
				const tile_adaptor_iterator tmp = *this;
				--(*this);
				return tmp;
			}

			constexpr tile_adaptor_iterator& operator+=(const difference_type difference) noexcept{
				data += difference;
				return *this;
			}

			constexpr tile_adaptor_iterator& operator-=(const difference_type difference) noexcept{
				data -= difference;
				return *this;
			}

			constexpr tile_adaptor_iterator operator+(const difference_type difference) const noexcept{
				return tile_adaptor_iterator{data + difference};
			}

			constexpr tile_adaptor_iterator operator-(const difference_type difference) const noexcept{
				return tile_adaptor_iterator{data - difference};
			}

			constexpr friend tile_adaptor_iterator operator+(const difference_type difference, const tile_adaptor_iterator& itr) noexcept{
				return tile_adaptor_iterator{itr.data + difference};
			}

			constexpr friend tile_adaptor_iterator operator-(const difference_type difference, const tile_adaptor_iterator& itr) noexcept{
				return tile_adaptor_iterator{itr.data - difference};
			}

			constexpr difference_type operator-(const tile_adaptor_iterator other) const noexcept{
				return std::distance(data, other.data);
			}

			decltype(auto) operator[](const difference_type idx) const noexcept{
				return data[idx];
			}

			decltype(auto) operator[](const difference_type idx) noexcept{
				return data[idx];
			}
		};

	public:
		using iterator = tile_adaptor_iterator<value_type>;
		using const_iterator = tile_adaptor_iterator<const value_type>;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;


		[[nodiscard]] constexpr tile_adaptor() noexcept = default;

		[[nodiscard]] constexpr bool empty() const noexcept{
			return width() * height() == 0;
		}

		[[nodiscard]] constexpr auto begin() noexcept{
			return iterator{data_impl()};
		}

		[[nodiscard]] constexpr auto begin() const noexcept{
			return const_iterator{data_impl()};
		}

		[[nodiscard]] constexpr auto end() noexcept{
			return iterator{data_impl() + size()};
		}

		[[nodiscard]] constexpr auto end() const noexcept{
			return const_iterator{data_impl() + size()};
		}

		[[nodiscard]] constexpr auto rbegin() noexcept{
			return reverse_iterator{end()};
		}

		[[nodiscard]] constexpr auto rend() noexcept{
			return reverse_iterator{begin()};
		}

		[[nodiscard]] constexpr auto rbegin() const noexcept{
			return const_reverse_iterator{end()};
		}

		[[nodiscard]] constexpr auto rend() const noexcept{
			return const_reverse_iterator{begin()};
		}

		[[nodiscard]] constexpr auto crbegin() const noexcept{
			return const_reverse_iterator{end()};
		}

		[[nodiscard]] constexpr auto crend() const noexcept{
			return const_reverse_iterator{begin()};
		}

		[[nodiscard]] constexpr auto cbegin() const noexcept{
			return begin();
		}

		[[nodiscard]] constexpr auto cend() const noexcept{
			return end();
		}

		[[nodiscard]] constexpr size_type size() const noexcept{
			return width() * height();
		}

		[[nodiscard]] constexpr std::size_t size_bytes() const noexcept{
			return size() * sizeof(value_type);
		}

		constexpr value_type& operator[](const size_type where) noexcept{
			assert(where < size());
			return data_impl()[where];
		}

		constexpr const value_type& operator[](const size_type where) const noexcept{
			assert(where < size());
			return data_impl()[where];
		}

		constexpr value_type& operator[](const position_acquireable<size_type> auto& pos) noexcept{
			return this->at(pos);
		}

		constexpr const value_type& operator[](const position_acquireable<size_type> auto& pos) const noexcept{
			return this->at(pos);
		}

		[[nodiscard]] constexpr value_type* data() noexcept{
			return data_impl();
		}

		[[nodiscard]] constexpr const value_type* data() const noexcept{
			return data_impl();
		}

		[[nodiscard]] constexpr value_type& at(const size_type w, const size_type h) noexcept{
			assert(w < width() && h < height());
			return data_impl()[this->to_index(w, h)];
		}

		[[nodiscard]] constexpr const value_type& at(const size_type w, const size_type h) const noexcept{
			assert(w < width() && h < height());
			return data_impl()[this->to_index(w, h)];
		}


		[[nodiscard]] constexpr value_type& at(const position_acquireable<size_type> auto& pos) noexcept{
			return this->at(pos.x, pos.y);
		}

		[[nodiscard]] constexpr const value_type& at(const position_acquireable<size_type> auto& pos) const noexcept{
			return this->at(pos.x, pos.y);
		}

		[[nodiscard]] constexpr value_type& operator[](const size_type x, const size_type y) noexcept{
			return this->at(x, y);
		}

		[[nodiscard]] constexpr const value_type& operator[](const size_type x, const size_type y) const noexcept{
			return this->at(x, y);
		}

		/**
		 * @brief row major
		 */
		[[nodiscard]] constexpr size_type to_index(const size_type x, const size_type y) const noexcept{
			assert(x < width() && y < height());
			return width() * y + x;
		}

		template <position_constructable<size_type> Rst = math::vector2<size_type>>
		[[nodiscard]] constexpr Rst from_index(const size_type idx) const noexcept{
			return Rst{idx % width(), idx / width()};
		}

		[[nodiscard]] static constexpr packed_index_type pack(const size_type x, const size_type y) noexcept{
			packed_index_type rst{};
			rst |= static_cast<packed_index_type>(x) << (sizeof(size_type) * 8);
			rst |= static_cast<packed_index_type>(y);
			return rst;
		}

		template <position_constructable<size_type> Rst = math::vector2<size_type>>
		[[nodiscard]] static constexpr Rst unpack(const packed_index_type pos) noexcept{
			const auto y = static_cast<size_type>(pos & static_cast<packed_index_type>(std::numeric_limits<size_type>::max()));
			const auto x = static_cast<size_type>((pos ^ static_cast<packed_index_type>(y)) >> sizeof(size_type) * 8);
			return Rst{x, y};
		}

		[[nodiscard]] constexpr auto area() const noexcept{
			return width() * height();
		}

		template <std::floating_point Fp = float>
		[[nodiscard]] constexpr Fp ratio() const noexcept{
			return static_cast<Fp>(width()) / static_cast<Fp>(height());
		}


		[[nodiscard]] constexpr auto to_byte_mdspan_row_major() noexcept{
			return std::mdspan{reinterpret_cast<std::byte*>(data()), height(), width() * sizeof(T)};
		}

		[[nodiscard]] constexpr auto to_byte_mdspan_row_major() const noexcept{
			return std::mdspan{reinterpret_cast<const std::byte*>(data()), height(), width() * sizeof(T)};
		}

		[[nodiscard]] constexpr auto to_mdspan_row_major() noexcept{
			return std::mdspan{data(), height(), width()};
		}

		[[nodiscard]] constexpr auto to_mdspan_row_major() const noexcept{
			return std::mdspan{data(), height(), width()};
		}

		[[nodiscard]] constexpr auto to_mdspan() noexcept{
			return to_mdspan_row_major();
		}

		[[nodiscard]] constexpr auto to_mdspan() const noexcept{
			return to_mdspan_row_major();
		}


		[[nodiscard]] constexpr std::span<value_type> to_span() noexcept{
			return std::span<value_type>{data(), size()};
		}

		[[nodiscard]] constexpr std::span<const value_type> to_span() const noexcept{
			return std::span<const value_type>{data(), size()};
		}

		[[nodiscard]] constexpr std::span<std::byte> to_byte_span() noexcept requires (std::is_trivially_copyable_v<value_type>){
			return std::as_writable_bytes(to_span());
		}

		[[nodiscard]] constexpr std::span<const std::byte> to_byte_span() const noexcept requires (std::is_trivially_copyable_v<value_type>){
			return std::as_bytes(to_span());
		}

		[[nodiscard]] constexpr std::span<const value_type> row_at(const size_type rowIndex) const noexcept{
			assert(rowIndex < height());
			return std::span<const value_type>{data() + rowIndex * width(), data() + (rowIndex + 1) * width()};
		}

		[[nodiscard]] constexpr std::span<value_type> row_at(const size_type rowIndex) noexcept{
			assert(rowIndex < height());
			return std::span<value_type>{data() + rowIndex * width(), data() + (rowIndex + 1) * width()};
		}

#ifdef __cpp_lib_ranges_stride
		[[nodiscard]] constexpr auto column_at(const size_type columnIndex) const noexcept{
			assert(columnIndex < width());

			return std::span<const value_type>{data() + columnIndex, size() - width() + 1} | std::views::stride(width());
		}

		[[nodiscard]] constexpr auto column_at(const size_type columnIndex) noexcept{
			assert(columnIndex < width());

			return std::span<value_type>{data() + columnIndex, size() - width() + 1} | std::views::stride(width());
		}

		[[nodiscard]] decltype(auto) sub_tile_adaptor_view(size_type x, size_type y, size_type w, size_type h) const noexcept{
			assert(x + w <= width());
			assert(y + h <= height());

			return
				std::span<const value_type>{data() + this->to_index(x, y), h * width()} | std::views::slide(w) |
				std::views::stride(width());
		}

		[[nodiscard]] decltype(auto) sub_tile_adaptor_view(size_type x, size_type y, size_type w, size_type h) noexcept{
			assert(x + w <= width());
			assert(y + h <= height());

			return
				std::span<value_type>{data() + this->to_index(x, y), h * width()} | std::views::slide(w) |
				std::views::stride(width());
		}


		[[nodiscard]] decltype(auto) sub_tile_adaptor_view(
			const position_acquireable<size_type> auto& src,
			const position_acquireable<size_type> auto& extent) const noexcept{
			return
				this->sub_tile_adaptor_view(src.x, src.y, extent.x, extent.y);
		}

		[[nodiscard]] decltype(auto) sub_tile_adaptor_view(
			const position_acquireable<size_type> auto& src,
			const position_acquireable<size_type> auto& extent) noexcept{

			return
				this->sub_tile_adaptor_view(src.x, src.y, extent.x, extent.y);
		}

#endif

		[[nodiscard]] constexpr size_type width() const noexcept{
			return static_cast<const Impl&>(*this).width();
		}

		[[nodiscard]] constexpr size_type height() const noexcept{
			return static_cast<const Impl&>(*this).height();
		}

		[[nodiscard]] constexpr math::vector2<size_type> extent() const noexcept{
			return {width(), height()};
		}

		explicit constexpr operator bool() const noexcept{
			return data_impl() != nullptr;
		}

		template <std::invocable<size_type, size_type> Prov>
			requires (std::is_assignable_v<value_type&, std::invoke_result_t<Prov, size_type, size_type>>)
		constexpr void fill(Prov prov)
			noexcept(
				std::is_nothrow_assignable_v<value_type, std::invoke_result_t<Prov, size_type, size_type>>
				&& std::is_nothrow_invocable_v<Prov, size_type, size_type>)
			requires std::is_move_assignable_v<value_type>{
			for(size_type y = 0; y != height(); ++y){
				for(size_type x = 0; x != width(); ++x){
					data_impl()[this->to_index(x, y)] = std::invoke(prov, x, y);
				}
			}
		}

		template <std::invocable<value_type&> Fn>
		constexpr void each(Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, value_type&>){
			for(value_type& t : *this){
				std::invoke(fn, t);
			}
		}

		template <std::invocable<value_type&, size_type, size_type> Fn>
		constexpr void each(Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, value_type&>){
			for(size_type y = 0; y != height(); ++y){
				for(size_type x = 0; x != width(); ++x){
					std::invoke(fn, data_impl()[this->to_index(x, y)], x, y);
				}
			}
		}

		template <std::invocable<const value_type&> Fn>
		constexpr void each(Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, value_type&>){
			for(const value_type& t : *this){
				std::invoke(fn, t);
			}
		}

		template <std::invocable<value_type&> Fn, std::predicate<value_type&> Pred>
		constexpr void each(Pred pred, Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, value_type&>){
			for(value_type& t : *this){
				if(std::invoke(pred, t)) std::invoke(fn, t);
			}
		}

		template <std::invocable<const value_type&> Fn, std::predicate<const value_type&> Pred>
		constexpr void each(Pred pred, Fn fn) const noexcept(std::is_nothrow_invocable_v<Fn, value_type&>){
			for(const value_type& t : *this){
				if(std::invoke(pred, t)) std::invoke(fn, t);
			}
		}

		template <typename Ty>
		void write(Ty* ptr) noexcept(std::is_nothrow_assignable_v<Ty, const value_type&>){
			if constexpr(std::is_trivially_copy_assignable_v<value_type>){
				std::memcpy(ptr, data(), size_bytes());
			} else{
				std::ranges::copy_n(data(), size(), ptr);
			}
		}

		[[nodiscard]] decltype(auto) sub_tile_adaptor(size_type x, size_type y, size_type w, size_type h) const{
			assert(x + w <= width());
			assert(y + h <= height());

			tile_adaptor tile_adaptor{w, h};

			for(std::size_t yIdx = 0; yIdx < h; ++yIdx){
				if constexpr(std::is_trivially_copy_assignable_v<value_type>){
					std::memcpy(tile_adaptor.data() + tile_adaptor.width() * yIdx, data() + (y + yIdx) * width() + x, w * sizeof(value_type));
				} else{
					std::ranges::copy_n(data() + (y + yIdx) * width() + x, w, tile_adaptor.data() + tile_adaptor.width() * yIdx);
				}
			}

			return tile_adaptor;
		}

		[[nodiscard]] decltype(auto) sub_tile_adaptor(
			const position_acquireable<size_type> auto& src,
			const position_acquireable<size_type> auto& extent) const{

			return this->sub_tile_adaptor(src.x, src.y, extent.x, extent.y);
		}

	protected:
		void copy(const value_type* o_data, const std::size_t size){
			if constexpr(std::is_trivially_copy_assignable_v<value_type>){
				std::memcpy(data(), o_data, size * sizeof(value_type));
			} else{
				std::ranges::copy_n(o_data, size, begin());
			}
		}

		[[nodiscard]] constexpr const value_type* data_impl() const noexcept{
			return static_cast<const Impl&>(*this).data_impl();
		}

		[[nodiscard]] constexpr value_type* data_impl() noexcept{
			return static_cast<Impl&>(*this).data_impl();
		}

	// protected:
	// 	size_type width_{};
	// 	size_type height_{};
	};

	template <typename T, std::unsigned_integral SizeType = unsigned, std::size_t MinAlign = 4>
	struct tile : public tile_adaptor<tile<T, SizeType, MinAlign>, T, SizeType>{
	private:
		using base = tile_adaptor<tile, T, SizeType>;
		using allocator_type = std::allocator<T>;

		friend base;

	public:
		static constexpr std::size_t original_alignment = alignof(T);
		static constexpr std::size_t alignment = std::max(std::bit_ceil(MinAlign), alignof(T));
		static constexpr bool custom_align = original_alignment != alignment;

		static constexpr std::align_val_t align{alignment};

		using value_type = T;
		using size_type = SizeType;

		using packed_index_type = std::conditional_t<
			sizeof(size_type) == 1, std::uint16_t, std::conditional_t<
				sizeof(size_type) == 2, std::uint32_t, std::conditional_t<
					sizeof(size_type) == 4, std::uint64_t, void>>>;


	protected:
		struct tile_iterator : std::contiguous_iterator_tag{
			using iterator_category = std::contiguous_iterator_tag;
			using value_type = value_type;
			using difference_type = std::ptrdiff_t;

			value_type* data{};

			[[nodiscard]] constexpr tile_iterator() = default;

			[[nodiscard]] constexpr explicit(false) tile_iterator(value_type* data)
				: data{data}{}

			constexpr friend bool operator==(const tile_iterator& a, const tile_iterator& b) noexcept{
				return a.data == b.data;
			}

			constexpr auto operator<=>(const tile_iterator& o) const noexcept{
				return data <=> o.data;
			}

			constexpr value_type& operator*() noexcept{
				return *data;
			}

			constexpr value_type& operator*() const noexcept{
				return *data;
			}

			constexpr value_type* operator->() noexcept{
				return data;
			}

			constexpr value_type* operator->() const noexcept{
				return data;
			}

			constexpr tile_iterator& operator++() noexcept{
				++data;
				return *this;
			}

			constexpr tile_iterator operator++(int) noexcept{
				const tile_iterator tmp = *this;
				++(*this);
				return tmp;
			}

			constexpr tile_iterator& operator--() noexcept{
				--data;
				return *this;
			}

			constexpr tile_iterator operator--(int) noexcept{
				const tile_iterator tmp = *this;
				--(*this);
				return tmp;
			}

			constexpr tile_iterator& operator+=(const difference_type difference) noexcept{
				data += difference;
				return *this;
			}

			constexpr tile_iterator& operator-=(const difference_type difference) noexcept{
				data -= difference;
				return *this;
			}

			constexpr tile_iterator operator+(const difference_type difference) const noexcept{
				return tile_iterator{data + difference};
			}

			constexpr tile_iterator operator-(const difference_type difference) const noexcept{
				return tile_iterator{data - difference};
			}

			constexpr friend tile_iterator operator+(const difference_type difference, const tile_iterator& itr) noexcept{
				return tile_iterator{itr.data + difference};
			}

			constexpr friend tile_iterator operator-(const difference_type difference, const tile_iterator& itr) noexcept{
				return tile_iterator{itr.data - difference};
			}

			constexpr difference_type operator-(const tile_iterator other) const noexcept{
				return std::distance(data, other.data);
			}

			decltype(auto) operator[](const difference_type idx) const noexcept{
				return data[idx];
			}

			decltype(auto) operator[](const difference_type idx) noexcept{
				return data[idx];
			}
		};

	public:
		using iterator = tile_iterator;
		using const_iterator = std::const_iterator<tile_iterator>;
		using reverse_iterator = std::reverse_iterator<tile_iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;


		[[nodiscard]] constexpr tile() noexcept = default;

		[[nodiscard]] constexpr tile(const size_type width, const size_type height) :
			width_{width}, height_{height}{
			tile::malloc(width * height);
		}

		[[nodiscard]] constexpr tile(const size_type width, const size_type height, const value_type& init)
			/*requires std::is_copy_assignable_v<value_type>*/ :
			tile{width, height}{
			std::ranges::fill(this->begin(), this->end(), init);
		}

		[[nodiscard]] constexpr tile(const size_type width, const size_type height, const value_type* data)
			/*requires std::is_copy_assignable_v<value_type>*/ :
			tile{width, height}{
			this->copy(data, this->size());
		}

		constexpr ~tile() noexcept{
			tile::free();
		}

		/*constexpr*/
		tile(const tile& other)
			: tile{other.width(), other.height()}{
			this->copy(other.data(), this->extent());
		}

		/*constexpr*/
		tile& operator=(const tile& other){
			if(this == &other) return *this;

			if(this->area() == other.area()){
				this->width_ = other.width_;
				this->height_ = other.height_;
			}else{
				this->operator=(tile{other.width(), other.height()});
			}

			this->copy(other.data(), this->extent());

			return *this;
		}

		constexpr tile(tile&& other) noexcept
			: width_{other.width_}, height_{other.height_},
			  raw{std::exchange(other.raw, {})}{}

		constexpr tile& operator=(tile&& other) noexcept{
			if(this == &other) return *this;
			std::swap(width_, other.width_);
			std::swap(height_, other.height_);
			std::swap(raw, other.raw);
			return *this;
		}

		[[nodiscard]] constexpr value_type* release() noexcept{
			width_ = height_ = 0;
			return std::exchange(raw, nullptr);
		}

		constexpr void clear() noexcept{
			width_ = height_ = 0;
			free();
			raw = nullptr;
		}

		constexpr void create(const size_type width, const size_type height){
			this->operator=(tile{width, height});
		}

	protected:
		constexpr void malloc(const std::size_t sz){
			if(sz == 0) return;

			//TODO ...
			// if constexpr (custom_align){
			// }else{
			// 	raw = std::allocator_traits<allocator_type>::allocate(alloc, sz);
			// }

			raw = static_cast<value_type*>(operator new[](sizeof(value_type) * sz, align));

			if constexpr(!std::is_trivially_constructible_v<value_type>){
				static_assert(std::is_default_constructible_v<value_type>);
				std::ranges::uninitialized_default_construct(this->begin(), this->end());
			}
		}

		constexpr void free() noexcept{
			if(!raw) return;

			if constexpr(!std::is_trivially_destructible_v<value_type>){
				std::ranges::destroy(this->begin(), this->end());
			}

			::operator delete [](raw, this->size_bytes(), align);

			//
			// if constexpr (custom_align){
			// }else{
			// 	std::allocator_traits<allocator_type>::deallocate(alloc, raw, this->size());
			// }
		}

	public:
		[[nodiscard]] constexpr size_type width() const noexcept{
			return width_;
		}

		[[nodiscard]] constexpr size_type height() const noexcept{
			return height_;
		}

	private:
		[[nodiscard]] constexpr const value_type* data_impl() const noexcept{
			return raw;
		}

		[[nodiscard]] constexpr value_type* data_impl() noexcept{
			return raw;
		}

		size_type width_{};
		size_type height_{};
		value_type* raw{};

		// ADAPTED_NO_UNIQUE_ADDRESS
		// allocator_type alloc{};
	};

}

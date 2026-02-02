export module mo_yanxi.bitmap;

export import mo_yanxi.math.vector2;

import std;
import mo_yanxi.concepts;
import mo_yanxi.dim2.tile;

namespace mo_yanxi{
export
struct alignas(sizeof(std::uint32_t)) color_bits{
	using bit_type = std::uint8_t;

	bit_type r;
	bit_type g;
	bit_type b;
	bit_type a;

	color_bits& operator=(const std::uint32_t val) noexcept{
		std::memcpy(this, &val, sizeof(std::uint32_t));
		return *this;
	}

	color_bits& operator&=(std::uint32_t val) noexcept{
		val &= std::bit_cast<std::uint32_t>(*this);
		std::memcpy(this, &val, sizeof(std::uint32_t));
		return *this;
	}

	color_bits& operator|=(std::uint32_t val) noexcept{
		val |= std::bit_cast<std::uint32_t>(*this);
		std::memcpy(this, &val, sizeof(std::uint32_t));
		return *this;
	}

	color_bits& operator^=(std::uint32_t val) noexcept{
		val |= std::bit_cast<std::uint32_t>(*this);
		std::memcpy(this, &val, sizeof(std::uint32_t));
		return *this;
	}

	color_bits operator~() const noexcept{
		return color_bits{} = ~std::bit_cast<std::uint32_t>(*this);
	}

	friend color_bits operator&(color_bits lhs, const std::uint32_t val) noexcept{
		return lhs &= val;
	}

	friend color_bits operator|(color_bits lhs, const std::uint32_t val) noexcept{
		return lhs |= val;
	}

	friend color_bits operator^(color_bits lhs, const std::uint32_t val) noexcept{
		return lhs ^= val;
	}

	template <std::integral I>
	[[nodiscard]] constexpr bit_type operator[](const I i) const noexcept{
		switch(i & 0b11u){
		case 0 : return r;
		case 1 : return g;
		case 2 : return b;
		case 3 : return a;
		default : std::unreachable();
		}
	}

	constexpr bool friend operator==(const color_bits&, const color_bits&) noexcept = default;
	constexpr auto operator<=>(const color_bits&) const noexcept = default;

	[[nodiscard]] constexpr std::uint32_t pack() const noexcept{
		return std::bit_cast<std::uint32_t>(*this);
	}

	explicit(false) constexpr operator std::uint32_t() const noexcept{
		return pack();
	}
};


export
struct bitmap : dim2::tile<color_bits>{
	static constexpr size_type channels = 4;
	using extent_type = math::vector2<size_type>;
	using point_type = math::vector2<size_type>;

	[[nodiscard]] bitmap() = default;

	[[nodiscard]] bitmap(size_type width, size_type height, const value_type& init)
	: tile<color_bits>(width, height, init){
	}

	[[nodiscard]] bitmap(size_type width, size_type height)
	: tile(width, height){
	}

	[[nodiscard]] bitmap(size_type width, size_type height, const value_type* data)
	: tile(width, height, data){
	}

	[[nodiscard]] constexpr extent_type extent() const noexcept{
		return tile::extent();
	}

	void mul_white() noexcept{
		for(auto& bits : *this){
			bits |= 0x00'ff'ff'ff;
		}
	}

	void load_unchecked(const value_type::bit_type* src_data, const bool flipY = false) noexcept{
		const auto dst = reinterpret_cast<value_type::bit_type*>(data());
		if(flipY){
			const size_type rowDataCount = width() * channels;

			for(size_type y = 0; y < height(); ++y){
				std::memcpy(dst + rowDataCount * y, src_data + (height() - y - 1) * rowDataCount, rowDataCount);
			}
		} else{
			std::memcpy(dst, src_data, size_bytes());
		}
	}
};


export
struct bitmap_view{
	std::mdspan<const color_bits, std::dextents<std::size_t, 2>> data{};

	[[nodiscard]] bitmap_view() = default;

	[[nodiscard]] explicit bitmap_view(const bitmap& data)
	: data(data.to_mdspan_row_major()){
	}
};

}

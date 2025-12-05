export module mo_yanxi.char_filter;

import std;

namespace mo_yanxi{


export
struct char_filter{
	using value_type = char;
private:
	std::bitset<std::numeric_limits<value_type>::max() - std::numeric_limits<value_type>::lowest()> map{};
public:
	[[nodiscard]] constexpr char_filter() = default;

	template <std::size_t N>
	constexpr explicit(false) char_filter(const value_type(&arr)[N], const bool flip = false) noexcept{
		for (auto c : arr){
			map.set(std::bit_cast<std::make_unsigned_t<value_type>>(c), true);
		}

		if(flip){
			map.flip();
		}
	}

	constexpr bool operator[](const value_type c) const noexcept{
		return map[std::bit_cast<std::make_unsigned_t<value_type>>(c)];
	}

};
}
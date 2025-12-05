//
// Created by Matrix on 2025/9/7.
//

export module mo_yanxi.format;
import std;

namespace mo_yanxi{
	template <typename CharT = char, std::size_t Sz = 1>
		requires (Sz > 0)
	struct mo_static_str{
		using value_type = CharT;
		value_type str[Sz]{};

		[[nodiscard]] constexpr mo_static_str() = default;

		[[nodiscard]] constexpr explicit(false) mo_static_str(const value_type (&str)[Sz]) noexcept{
			for(std::size_t i = 0; i < Sz; ++i){
				this->str[i] = str[i];
			}
		}

		constexpr explicit(false) operator std::basic_string_view<value_type>() const noexcept{
			return {str, Sz - 1};
		}

		static constexpr std::size_t size() noexcept{
			return Sz;
		}
	};


	template <typename T, std::size_t digits_per_split, auto ...names>
		requires (std::formattable<decltype(names), char> && ...)
	struct metric_prefix_formatter{
	private:
		static constexpr std::size_t max_split = sizeof...(names);
		static constexpr std::tuple names_args{names...};
		static constexpr std::size_t split = []{
			std::size_t t = 1;
			for (std::size_t i = 0; i < digits_per_split; ++i){
				t *= 10uz;
			}
			return t;
		}();
		static constexpr T same_type_split = static_cast<T>(split);
		static constexpr double double_split = static_cast<double>(split);

	public:
		T val;

		template <typename OutputIt>
		OutputIt operator ()(OutputIt context_itr, unsigned precision) const{
			if(val < same_type_split){
				if constexpr (std::floating_point<T>){
					return std::format_to(context_itr, "{:g<.{}f}", val, precision);
				}else{
					return std::format_to(context_itr, "{}", val);
				}

			}

			std::optional<OutputIt> out{};
			double v = val / double_split;
			[&]<std::size_t ... Idx>(std::index_sequence<Idx...>){
				([&]<std::size_t I>(){
					if(out) return;
					if(v < double_split){
						out = std::format_to(context_itr, "{:g<.{}f}{}", v, precision, std::get<Idx>(names_args));
					} else{
						v /= double_split;
					}
				}.template operator()<Idx>(), ...);
			}(std::make_index_sequence<max_split - 1>{});

			if(!out){
				out = std::format_to(context_itr, "{:g<.{}f}{}", v, precision, std::get<max_split - 1>(names_args));
			}

			return out.value();
		}
	};
}


template <typename CharT, std::size_t Sz>
struct std::formatter<mo_yanxi::mo_static_str<CharT, Sz>>{
	static constexpr auto parse(std::basic_format_parse_context<CharT>& context){
		return context.begin();
	}

	template <typename OutputIt>
	OutputIt format(const mo_yanxi::mo_static_str<CharT, Sz>& c, std::basic_format_context<OutputIt, CharT>& context) const{
		return std::format_to(context.out(), "{}", static_cast<std::basic_string_view<CharT>>(c));
	}
};

template <typename T, std::size_t digits_per_split, auto ...names>
struct std::formatter<mo_yanxi::metric_prefix_formatter<T, digits_per_split, names...>>{
	unsigned precision{1u};

	constexpr auto parse(std::format_parse_context& context){
		auto it = context.begin();
		auto end = context.end();

		std::optional<unsigned> spec_precision{};
		while (it != end && (*it >= '0' && *it <= '9')) {
			spec_precision = spec_precision.value_or(0) * 10 + (*it - '0');
			++it;
		}
		if(spec_precision){
			precision = *spec_precision;
		}

		if(it != end && *it != '}') throw std::format_error("Invalid format");

		return it;
	}

	template <typename OutputIt, typename CharT>
	auto format(const mo_yanxi::metric_prefix_formatter<T, digits_per_split, names...>& c, std::basic_format_context<OutputIt, CharT>& context) const{
		return c.operator()(context.out(), precision);
	}
};


namespace mo_yanxi{
	export
	template <typename T>
		requires (std::is_arithmetic_v<T>)
	[[nodiscard]] constexpr auto make_unipfx(T arth) noexcept ->
	metric_prefix_formatter<T, 3, mo_static_str{"K"}, mo_static_str{"M"}, mo_static_str{"B"}, mo_static_str{"T"}, mo_static_str{"Qa"}, mo_static_str{"Qi"}> {
		return {arth};
	}
}

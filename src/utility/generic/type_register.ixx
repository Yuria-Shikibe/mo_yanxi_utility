export module mo_yanxi.type_register;

import std;

namespace mo_yanxi{
	export
	template <typename T>
	consteval std::string_view name_of() noexcept {
		static constexpr auto loc = std::source_location::current();
		constexpr std::string_view sv{loc.function_name()};

#ifdef __clang__
		constexpr std::string_view prefix{"[T = "};
		auto pos = sv.find(prefix) + prefix.size();
		return sv.substr(pos, sv.size() - pos - 1);
#else
		constexpr auto pos = sv.find(__func__) + 1;

		constexpr auto remain = sv.substr(pos);
		constexpr auto initial_pos = remain.find('<') + 1;

		constexpr auto i = [](std::size_t initial, std::string_view str) static consteval {
			std::size_t count = 1;
			for(; initial < str.size(); ++initial){
				if(str[initial] == '<')count++;
				if(str[initial] == '>')count--;
				if(!count)break;
			}

			return initial;
		}(initial_pos, remain);

		constexpr std::size_t size = i - initial_pos;
		return remain.substr(initial_pos, size);
#endif
	}
	export
	struct type_identity_data;

	export using type_identity_index = const type_identity_data*;

	template <typename T>
	[[nodiscard]] constexpr type_identity_index unstable_type_identity_of_impl() noexcept;

	export
	struct type_identity_data{
	private:
		using Fn = std::type_index() noexcept;
		std::string_view name_;
		Fn* fn{};

	public:
		template <typename T>
		[[nodiscard]] constexpr explicit(false) type_identity_data(std::in_place_type_t<T>) : name_(mo_yanxi::name_of<T>()), fn{+[] static noexcept {
			return std::type_index(typeid(T));
		}}{

		}

		[[nodiscard]] constexpr std::type_index type_idx() const noexcept{
			return fn();
		}

		[[nodiscard]] constexpr std::string_view name() const noexcept{
			return name_;
		}
	};

	template <typename T>
	inline constexpr type_identity_data idt{std::in_place_type<T>};

	template <typename T>
	[[nodiscard]] constexpr type_identity_index unstable_type_identity_of_impl() noexcept{
		return std::addressof(idt<T>);
	}

	export
	template <typename T>
	[[nodiscard]] constexpr type_identity_index unstable_type_identity_of() noexcept{
		return unstable_type_identity_of_impl<std::remove_cvref_t<T>>();
	}
}


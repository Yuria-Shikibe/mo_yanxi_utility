export module mo_yanxi.type_map;

import std;
import mo_yanxi.meta_programming;

export namespace mo_yanxi{
	template <typename V, typename Ts>
		requires (std::is_default_constructible_v<V>)
	struct type_map : type_to_index<Ts>{
	private:
		using base = type_to_index<Ts>;

	public:
		using value_type = V;

		std::array<V, base::arg_size> items{};

		template <typename S, typename Fn>
		void each(this S&& self, Fn fn){
			[&]<std::size_t ...I>(std::index_sequence<I...>){
				([&]<std::size_t Idx>(){
					using T = typename base::template arg_at<Idx>;
					if constexpr (std::invocable<Fn, std::in_place_type_t<T>, copy_qualifier_t<S&&, value_type>>){
						std::invoke(fn, std::in_place_type<T>, std::forward_like<S>(self.items[I]));
					}else if constexpr (std::invocable<Fn, copy_qualifier_t<S&&, value_type>>){
						std::invoke(fn, std::forward_like<S>(self.items[I]));
					}
				}.template operator()<I>(), ...);
			}(std::make_index_sequence<base::arg_size>{});
		}

		template <std::size_t I>
		[[nodiscard]] constexpr auto& at() const noexcept{
			static_assert(I < base::arg_size, "invalid index");

			return items[I];
		}

		template <std::size_t I>
		[[nodiscard]] constexpr auto& at() noexcept{
			static_assert(I < base::arg_size, "invalid index");

			return items[I];
		}

		template <typename Ty>
		[[nodiscard]] constexpr auto& at() const noexcept{
			return at<base::template index_of<Ty>>();
		}

		template <typename Ty>
		[[nodiscard]] constexpr auto& at() noexcept{
			return at<base::template index_of<Ty>>();
		}
	};

	template <typename V, typename ...Ts>
	using type_map_of = type_map<V, std::tuple<Ts...>>;
}

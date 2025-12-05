module;

#include <cassert>

export module mo_yanxi.refable_tuple;

import mo_yanxi.concepts;
import std;

namespace mo_yanxi{
	template <class T>
		class def_reference_wrapper{
	public:
		static_assert(std::is_object_v<T> || std::is_function_v<T>,
			"reference_wrapper<T> requires T to be an object type or a function type.");

		using type = T;

		[[nodiscard]] constexpr def_reference_wrapper() = default;

		template <class U>
			requires (std::same_as<std::remove_cvref_t<U>, T> || std::same_as<std::add_const_t<std::remove_cvref_t<U>>, T>)
		explicit(false) constexpr def_reference_wrapper(U&& v){
			T& r = static_cast<U&&>(v);
			ptr      = std::addressof(r);
		}

		template <class U>
			requires (std::same_as<U, T> || std::same_as<std::add_const_t<U>, T>)
		explicit(false) constexpr def_reference_wrapper(std::reference_wrapper<U> v){
			ptr = std::addressof(v.get());
		}

		constexpr operator T&() const noexcept {
			assert(ptr);
			return *ptr;
		}

		[[nodiscard]] constexpr T& get() const noexcept {
			assert(ptr);
			return *ptr;
		}

	private:
		T* ptr{};
	};


	export
	template <typename Ty>
	using refable_tuple_decay_ref = std::conditional_t<std::is_lvalue_reference_v<Ty>, def_reference_wrapper<std::remove_reference_t<Ty>>, std::decay_t<Ty>>;

	template <typename Tuple>
	constexpr auto decay(){
		if constexpr (std::tuple_size_v<Tuple> == 0){
			return std::tuple<>{};
		}else{
			return [] <std::size_t... Indices>(std::index_sequence<Indices...>){
				// static_assert((std::is_rvalue_reference_v<std::tuple_element_t<Indices, Tuple>> && ...),
				// 	"ref decay tuple should not include rvalue_reference");
				//
				return std::tuple<refable_tuple_decay_ref<std::tuple_element_t<Indices, Tuple>> ...>{};
			}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
		}
	}

	export
	template <typename Tuple>
		requires (is_tuple_v<Tuple>)
	using to_refable_tuple_t = decltype(decay<Tuple>());

	export
	template <typename ...Ts>
	using refable_tuple = to_refable_tuple_t<std::tuple<Ts...>>;

	export
	template <typename Fn, typename Tuple>
		requires (is_tuple_v<std::remove_cvref_t<Tuple>>)
	decltype(auto) apply_refable_tuple(Fn fn, Tuple&& tuple){
		return [&] <std::size_t ...Idx>(std::index_sequence<Idx...>){
			return std::invoke(fn, std::forward_like<Tuple>(std::get<Idx>(tuple)) ...);
		}(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>>>{});
	}

	/**
	 * @brief lref wrapped with std::ref/cref collapse into lref, otherwise decay to value
	 */
	export
	template <typename ...Ts>
	constexpr refable_tuple<refable_tuple_decay_ref<std::unwrap_ref_decay_t<Ts>> ...> make_refable_tuple(Ts&& ...args) noexcept{
		return refable_tuple<refable_tuple_decay_ref<std::unwrap_ref_decay_t<Ts>> ...>{std::forward<Ts>(args) ...};
	}

}

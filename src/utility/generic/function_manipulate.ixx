module;

#include <mo_yanxi/adapted_attributes.hpp>

export module mo_yanxi.function_manipulate;

import std;
export import mo_yanxi.meta_programming;

namespace mo_yanxi{
export
template <typename Fn, typename Base>
concept func_initializer_of = requires{
	requires mo_yanxi::is_complete<function_traits<std::decay_t<Fn>>>;
	typename function_arg_at_last<std::decay_t<Fn>>::type;
	requires std::is_lvalue_reference_v<typename function_arg_at_last<std::decay_t<Fn>>::type>;
	requires std::derived_from<std::remove_cvref_t<typename function_arg_at_last<std::decay_t<Fn>>::type>, Base>;
};

export
template <typename Fn, typename Base, typename... Args>
concept invocable_func_initializer_of = requires{
	requires func_initializer_of<Fn, Base>;
	requires std::invocable<
		Fn,
		std::add_lvalue_reference_t<std::remove_cvref_t<typename function_arg_at_last<std::decay_t<Fn>>::type>>,
		Args...
	>;
};

export
template <typename InitFunc>
struct func_initializer_trait{
	using target_type = std::remove_cvref_t<typename function_arg_at_last<std::decay_t<InitFunc>>::type>;
};

#pragma region RedundantCall

template <typename Param>
struct passable_probe{
	Param value;
};

template <typename Param, typename Arg>
concept passable = requires(Arg&& arg){
	// Use list-initialization to reject narrowing conversions during redundant argument matching.
	passable_probe<Param>{std::forward<Arg>(arg)};
};

template <typename... Ts>
struct unique_decayed_pack : std::true_type{
};

template <typename T, typename... Rest>
struct unique_decayed_pack<T, Rest...>
	: std::bool_constant<(!std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<Rest>> && ...)
	                     && unique_decayed_pack<Rest...>::value>{
};

template <typename Tuple>
struct unique_decayed_tuple;

template <typename... Ts>
struct unique_decayed_tuple<std::tuple<Ts...>> : unique_decayed_pack<Ts...>{
};

template <typename SubArgsTuple, typename FullTuple>
consteval auto get_subset_indices();

template <std::size_t... Is>
consteval bool has_unique_indices(std::index_sequence<Is...>){
	constexpr std::size_t values[] = {Is...};
	for(std::size_t i = 0; i < sizeof...(Is); ++i){
		for(std::size_t j = i + 1; j < sizeof...(Is); ++j){
			if(values[i] == values[j]){
				return false;
			}
		}
	}

	return true;
}

template <typename Target, typename FullTuple>
consteval std::size_t count_passable_matches(){
	static constexpr std::size_t N = std::tuple_size_v<FullTuple>;
	return [&]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
		return (std::size_t{} + ... + static_cast<std::size_t>(passable<Target, std::tuple_element_t<Is, FullTuple>>));
	}(std::make_index_sequence<N>{});
}

template <typename Target, typename FullTuple>
consteval std::size_t first_passable_index(){
	static constexpr std::size_t N = std::tuple_size_v<FullTuple>;

	return [&]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
		std::size_t found_idx = N;
		((passable<Target, std::tuple_element_t<Is, FullTuple>>
			? (found_idx = Is, true)
			: false) || ...);
		return found_idx;
	}(std::make_index_sequence<N>{});
}

template <typename SubArgsTuple, typename FullTuple>
consteval bool has_valid_subset_mapping(){
	if constexpr(!unique_decayed_tuple<FullTuple>::value){
		return false;
	} else{
		static constexpr std::size_t N = std::tuple_size_v<SubArgsTuple>;
		constexpr auto indices = []<std::size_t... Is>(std::index_sequence<Is...>){
			return std::index_sequence<
				first_passable_index<std::tuple_element_t<Is, SubArgsTuple>, FullTuple>()...
			>{};
		}(std::make_index_sequence<N>{});

		return []<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
			return ((count_passable_matches<std::tuple_element_t<Is, SubArgsTuple>, FullTuple>() == 1) && ...)
			    && has_unique_indices(indices);
		}(std::make_index_sequence<N>{});
	}
}

template <typename Fn, typename FullTuple, std::size_t... Is>
consteval bool subset_invocable_with_indices(std::index_sequence<Is...>){
	return std::invocable<Fn, std::tuple_element_t<Is, FullTuple>...>;
}

template <typename Fn, typename SubArgsTuple, typename FullTuple>
consteval bool is_redundantly_invocable_with_tuple(){
	if constexpr(!has_valid_subset_mapping<SubArgsTuple, FullTuple>()){
		return false;
	} else{
		constexpr auto indices = get_subset_indices<SubArgsTuple, FullTuple>();
		return mo_yanxi::subset_invocable_with_indices<Fn, FullTuple>(indices);
	}
}

template <typename Target, typename FullTuple>
consteval std::size_t find_decayed_index(){
	static_assert(count_passable_matches<Target, FullTuple>() == 1,
	              "Target argument must match exactly one FullArg");
	return first_passable_index<Target, FullTuple>();
}

template <typename SubArgsTuple, typename FullTuple>
consteval auto get_subset_indices(){
	static_assert(unique_decayed_tuple<FullTuple>::value,
	              "Redundant call requires FullArgs to have distinct decayed types");
	static constexpr std::size_t N = std::tuple_size_v<SubArgsTuple>;
	constexpr auto indices = []<std::size_t... Is>(std::index_sequence<Is...>){
		return std::index_sequence<
			find_decayed_index<std::tuple_element_t<Is, SubArgsTuple>, FullTuple>()...
		>{};
	}(std::make_index_sequence<N>{});

	static_assert(has_unique_indices(indices),
	              "Redundant call requires each target argument to bind to a distinct FullArg");
	return indices;
}

template <typename Fn, typename FullTuple, std::size_t... Is>
constexpr decltype(auto) invoke_with_indices(Fn&& f, FullTuple&& full_args, std::index_sequence<Is...>){
	return std::invoke(std::forward<Fn>(f), std::get<Is>(std::forward<FullTuple>(full_args))...);
}

export
template <typename Fn, typename... FullArgs>
concept redundantly_invocable = unique_decayed_pack<FullArgs...>::value
	&& (std::invocable<Fn&&, FullArgs&&...>
	    || (unambiguous_function<std::decay_t<Fn>>
	        && is_redundantly_invocable_with_tuple<Fn&&,
	                                               typename function_traits<std::decay_t<Fn>>::mem_func_args_type,
	                                               std::tuple<FullArgs...>>()));


template <typename T, typename Tuple>
struct call_traits_helper;

template <typename T, typename... SubArgs>
struct call_traits_helper<T, std::tuple<SubArgs...>>{
	// 测试是否具有 static operator()，只用子集参数测试
	static constexpr bool is_static_op = requires(SubArgs... args){
		{ T::operator()(std::forward<SubArgs>(args)...) };
	};

	// 测试是否支持 const 调用，利用标准库的 std::invocable 即可，最严谨
	static constexpr bool is_const_op = std::invocable<const std::remove_cvref_t<T>&, SubArgs...>;
};

export
template <typename... FullArgs, typename Fn>
constexpr auto make_func_wrapper(Fn&& fn) noexcept(std::is_nothrow_constructible_v<std::decay_t<Fn>, Fn&&>){
	static_assert(unique_decayed_pack<FullArgs...>::value,
	              "make_func_wrapper requires FullArgs to have distinct decayed types");

	if constexpr(std::invocable<Fn&&, FullArgs...>){
		return std::forward<Fn>(fn);
	}

	using DecayedFn = std::decay_t<Fn>;

	using SubArgsTuple = typename function_traits<DecayedFn>::mem_func_args_type;

	constexpr static auto indices = get_subset_indices<SubArgsTuple, std::tuple<FullArgs...>>();

	constexpr static bool is_empty = std::is_empty_v<DecayedFn>;

	using CallTraits = call_traits_helper<DecayedFn, SubArgsTuple>;
	constexpr static bool is_static_op = CallTraits::is_static_op;
	constexpr static bool is_const_op = CallTraits::is_const_op;

	if constexpr(is_empty){
		return []ATTR_FORCEINLINE_SENTENCE (FullArgs... args) static -> decltype(auto){
			auto full_tuple = std::forward_as_tuple(std::forward<FullArgs>(args)...);
			return mo_yanxi::invoke_with_indices(DecayedFn{}, std::move(full_tuple), indices);
		};
	} else if constexpr(is_static_op){
		return [] ATTR_FORCEINLINE_SENTENCE (FullArgs... args) static -> decltype(auto){
			auto full_tuple = std::forward_as_tuple(std::forward<FullArgs>(args)...);
			return mo_yanxi::invoke_with_indices([]<typename... Ts>(Ts&&... sub_args) -> decltype(auto){
				return DecayedFn::operator()(std::forward<Ts>(sub_args)...);
			}, std::move(full_tuple), indices);
		};
	} else{
		if constexpr(is_const_op){
			return [f = std::forward<Fn>(fn)] ATTR_FORCEINLINE_SENTENCE (FullArgs... args) -> decltype(auto){
				auto full_tuple = std::forward_as_tuple(std::forward<FullArgs>(args)...);

				return mo_yanxi::invoke_with_indices(f, std::move(full_tuple), indices);
			};
		} else{
			return [f = std::forward<Fn>(fn)] ATTR_FORCEINLINE_SENTENCE (FullArgs... args) mutable -> decltype(auto){
				auto full_tuple = std::forward_as_tuple(std::forward<FullArgs>(args)...);
				return mo_yanxi::invoke_with_indices(f, std::move(full_tuple), indices);
			};
		}
	}
}

export
template <typename... FullArgs, typename Fn>
constexpr decltype(auto) invoke_redundantly(Fn&& fn, FullArgs&&... args){
	static_assert(unique_decayed_pack<FullArgs...>::value,
	              "invoke_redundantly requires FullArgs to have distinct decayed types");
	static_assert(redundantly_invocable<Fn, FullArgs...>,
	              "invoke_redundantly requires a function invocable directly or by redundant argument matching");

	if constexpr(std::invocable<Fn&&, FullArgs&&...>){
		return std::invoke(std::forward<Fn>(fn), std::forward<FullArgs>(args)...);
	}

	using DecayedFn = std::decay_t<Fn>;
	using SubArgsTuple = typename function_traits<DecayedFn>::mem_func_args_type;

	constexpr static auto indices = get_subset_indices<SubArgsTuple, std::tuple<FullArgs...>>();
	constexpr static bool is_empty = std::is_empty_v<DecayedFn>;
	constexpr static bool is_static_op = call_traits_helper<DecayedFn, SubArgsTuple>::is_static_op;

	auto full_tuple = std::forward_as_tuple(std::forward<FullArgs>(args)...);

	if constexpr(is_empty){
		ATTR_FORCEINLINE_SENTENCE
		return mo_yanxi::invoke_with_indices(DecayedFn{}, std::move(full_tuple), indices);
	} else if constexpr(is_static_op){
		ATTR_FORCEINLINE_SENTENCE
		return mo_yanxi::invoke_with_indices([]<typename... Ts>(Ts&&... sub_args) -> decltype(auto){
			return DecayedFn::operator()(std::forward<Ts>(sub_args)...);
		}, std::move(full_tuple), indices);
	} else{
		ATTR_FORCEINLINE_SENTENCE
		return mo_yanxi::invoke_with_indices(std::forward<Fn>(fn), std::move(full_tuple), indices);
	}
}

#pragma endregion

}

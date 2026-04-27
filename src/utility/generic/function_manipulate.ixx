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
}

import std;

namespace mo_yanxi{

template <typename T, typename R>
concept passable = std::convertible_to<std::add_pointer_t<T>, std::add_pointer_t<R>> && std::constructible_from<R, T&&>;

template <typename Target, typename FullTuple>
consteval std::size_t find_decayed_index(){
	using TargetTy = std::remove_cvref_t<Target>;
	static constexpr std::size_t N = std::tuple_size_v<FullTuple>;

	static constexpr auto rst = [&]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
		std::size_t found_idx = N;
		bool found = ( (passable<TargetTy, std::tuple_element_t<Is, FullTuple>> ? (found_idx = Is, true) : false) || ...);
		return std::make_pair(found_idx, found);
	}(std::make_index_sequence<N>{});

	static_assert(rst.second, "Target Type Not Found");
	return rst.first;
}

template <typename SubArgsTuple, typename FullTuple>
consteval auto get_subset_indices(){
	static constexpr std::size_t N = std::tuple_size_v<SubArgsTuple>;
	return []<std::size_t... Is>(std::index_sequence<Is...>){
		return std::index_sequence<
			find_decayed_index<std::tuple_element_t<Is, SubArgsTuple>, FullTuple>()...
		>{};
	}(std::make_index_sequence<N>{});
}

template <typename Fn, typename FullTuple, std::size_t... Is>
constexpr decltype(auto) invoke_with_indices(Fn&& f, FullTuple&& full_args, std::index_sequence<Is...>){
	return std::invoke(std::forward<Fn>(f), std::get<Is>(std::forward<FullTuple>(full_args))...);
}


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
}

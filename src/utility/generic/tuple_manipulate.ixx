//
// Created by Matrix on 2025/11/18.
//

export module mo_yanxi.tuple_manipulate;

import std;

namespace mo_yanxi{
export
template <typename T>
struct is_tuple : std::false_type{
};

template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type{
};

export
template <typename T>
constexpr bool is_tuple_v = is_tuple<T>::value;

export
template <typename T, T a, T b>
constexpr inline T max_const = a > b ? a : b;

export
template <typename T, T a, T b>
constexpr inline T min_const = a < b ? a : b;


export
template <typename T>
struct unwrap;

template <template <typename...> typename W, typename... T>
struct unwrap<W<T...>> : std::type_identity<std::tuple<T...>>{
};

template <template <typename...> typename W, typename... T>
struct unwrap<const W<T...>> : std::type_identity<std::tuple<T...>>{
};

template <template <typename...> typename W, typename... T>
struct unwrap<volatile W<T...>> : std::type_identity<std::tuple<T...>>{
};

template <template <typename...> typename W, typename... T>
struct unwrap<const volatile W<T...>> : std::type_identity<std::tuple<T...>>{
};

export
template <typename T>
using unwrap_t = typename unwrap<T>::type;

export
template <std::size_t Idx, typename T>
using unwrap_element_t = std::tuple_element_t<Idx, typename unwrap<T>::type>;

export
template <typename T>
using unwrap_first_element_t = std::tuple_element_t<0, typename unwrap<T>::type>;

template <template <typename...> typename W, typename T>
consteval auto push_all_to() noexcept{
	return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
		return std::type_identity<W<std::tuple_element_t<Idx, T>...>>{};
	}(std::make_index_sequence<std::tuple_size_v<T>>{});
}

export
template <template <typename...> typename W, typename T>
	requires (is_tuple_v<T>)
using all_apply_to = typename decltype(push_all_to<W, T>())::type;


template <std::size_t i, template <typename > typename UnaryPred, typename Tuple>
constexpr void modify(std::size_t& index){
	if(index != std::tuple_size_v<Tuple>) return;
	if constexpr(i >= std::tuple_size_v<Tuple>){
		return;
	} else if constexpr(UnaryPred<std::tuple_element_t<i, Tuple>>::value){
		index = i;
	}
}

export
template <template <typename > typename UnaryPred, typename Tuple>
constexpr inline std::size_t tuple_find_first_v = []() constexpr -> std::size_t{
	std::size_t index = std::tuple_size_v<Tuple>;
	[&]<std::size_t... Idx>(std::index_sequence<Idx...>){
		(modify<Idx, UnaryPred, Tuple>(index), ...);
	}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

	return index;
}();

template <std::size_t i, template <typename, typename > typename BinaryPred, typename Tuple, typename T>
constexpr void modify(std::size_t& index){
	if(index != std::tuple_size_v<Tuple>) return;
	if constexpr(i >= std::tuple_size_v<Tuple>){
		return;
	} else if constexpr(BinaryPred<std::tuple_element_t<i, Tuple>, T>::value){
		index = i;
	}
}

export
template <template <typename, typename > typename BinaryPred, typename Tuple, typename T>
constexpr inline std::size_t tuple_match_first_v = []() constexpr -> std::size_t{
	std::size_t index = std::tuple_size_v<Tuple>;
	[&]<std::size_t... Idx>(std::index_sequence<Idx...>){
		(modify<Idx, BinaryPred, Tuple, T>(index), ...);
	}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

	return index;
}();

export
template <typename Tuple>
using reverse_tuple_t = typename decltype([]{
	if constexpr(std::tuple_size_v<Tuple> == 0){
		return std::type_identity<std::tuple<>>{};
	} else{
		return []<std::size_t... Idx>(std::index_sequence<Idx...>){
			return std::type_identity<std::tuple<std::tuple_element_t<std::tuple_size_v<Tuple> - 1 - Idx, Tuple>...>>{};
		}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
	}
}())::type;

export
template <template <typename > typename UnaryPred, typename Tuple>
struct tuple_find_first : std::integral_constant<std::size_t, tuple_find_first_v<UnaryPred, Tuple>>{
};


template <template <typename > typename W, typename T>
consteval auto apply_to() noexcept{
	return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
		return std::type_identity<std::tuple<W<std::tuple_element_t<Idx, T>>...>>{};
	}(std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <typename T>
consteval auto apply_ext_const_to_inner() noexcept{
	return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
		return std::type_identity<
			std::tuple<std::conditional_t<std::is_const_v<T>, std::add_const_t<std::tuple_element_t<Idx,
				std::remove_const_t<T>>>, std::tuple_element_t<Idx, T>>...>>{};
	}(std::make_index_sequence<std::tuple_size_v<T>>{});
}

export
template <template <typename > typename UnaryTrait, typename T>
using unary_apply_to_tuple_t = typename decltype(apply_to<UnaryTrait, T>())::type;

export
template <typename T>
using tuple_const_to_inner_t = typename decltype(apply_ext_const_to_inner<T>())::type;

constexpr std::size_t a = tuple_find_first_v<std::is_floating_point, std::tuple<int, float>>;

export
template <template <typename, typename> typename W, typename LHS, typename RHS>
using binary_apply_to_tuple_t = typename decltype([]{
	if constexpr(is_tuple_v<LHS> && is_tuple_v<RHS>){
		return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
			return std::type_identity<std::tuple<W<std::tuple_element_t<Idx, LHS>, std::tuple_element_t<Idx, RHS>>...>>
				{};
		}(std::make_index_sequence<
			min_const<std::size_t, std::tuple_size_v<LHS>, std::tuple_size_v<RHS>>
		>{});
	} else if constexpr(is_tuple_v<LHS>){
		return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
			return std::type_identity<std::tuple<W<std::tuple_element_t<Idx, LHS>, RHS>...>>{};
		}(std::make_index_sequence<std::tuple_size_v<LHS>>{});
	} else if constexpr(is_tuple_v<RHS>){
		return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
			return std::type_identity<std::tuple<W<LHS, std::tuple_element_t<Idx, RHS>>...>>{};
		}(std::make_index_sequence<std::tuple_size_v<RHS>>{});
	} else{
		static_assert(false, "At least one side should be tuple");
	}
}())::type;

template <std::size_t count, typename Tuple>
consteval auto remove_tuple_first_elem(){
	if constexpr(std::tuple_size_v<Tuple> <= count){
		return std::type_identity<std::tuple<>>{};
	}

	return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
		return std::type_identity<std::tuple<std::tuple_element_t<Idx + count, Tuple>...>>{};
	}(std::make_index_sequence<std::tuple_size_v<Tuple> - count>{});
}

template <std::size_t count, typename Tuple>
consteval auto remove_tuple_last_elem(){
	if constexpr(std::tuple_size_v<Tuple> <= count){
		return std::type_identity<std::tuple<>>{};
	}

	return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
		return std::type_identity<std::tuple<std::tuple_element_t<Idx, Tuple>...>>{};
	}(std::make_index_sequence<std::tuple_size_v<Tuple> - count>{});
}


template <std::size_t count, typename Tuple>
consteval auto take_tuple_first_elem(){
	if constexpr(std::tuple_size_v<Tuple> <= count){
		return std::type_identity<Tuple>{};
	} else{
		return [] <std::size_t ... Idx>(std::index_sequence<Idx...>){
			return std::type_identity<std::tuple<std::tuple_element_t<Idx, Tuple>...>>{};
		}(std::make_index_sequence<count>{});
	}
}


export
template <std::size_t Count, typename Tuple>
struct tuple_drop_front_n_elem : decltype(remove_tuple_first_elem<Count, Tuple>()){
};

export
template <std::size_t Count, typename Tuple>
struct tuple_drop_back_n_elem : decltype(remove_tuple_last_elem<Count, Tuple>()){
};

export
template <std::size_t Count, typename Tuple>
using tuple_drop_front_n_elem_t = typename tuple_drop_front_n_elem<Count, Tuple>::type;

export
template <std::size_t Count, typename Tuple>
using tuple_drop_back_n_elem_t = typename tuple_drop_back_n_elem<Count, Tuple>::type;

export
template <std::size_t Count, typename Tuple>
using tuple_take_n_elem_t = typename decltype(take_tuple_first_elem<Count, Tuple>())::type;

export
template <typename Tuple>
using tuple_drop_first_elem_t = typename decltype(remove_tuple_first_elem<1, Tuple>())::type;

export
template <typename... Tuples>
using tuple_cat_t = decltype(std::tuple_cat(std::declval<Tuples&&>()...));

export
template <typename Tuple>
using tuple_drop_last_elem_t = typename decltype(remove_tuple_last_elem<1, Tuple>())::type;


template <typename T, typename Tuple, size_t Index = 0>
consteval std::size_t find_first_index_in_tuple(){
	if constexpr(Index >= std::tuple_size_v<Tuple>){
		return Index;
	} else if constexpr(std::same_as<T, std::tuple_element_t<Index, Tuple>>){
		return Index;
	} else{
		return find_first_index_in_tuple<T, Tuple, Index + 1>();
	}
}

export
template <typename T, typename Tuple>
constexpr std::size_t tuple_index_v = find_first_index_in_tuple<T, Tuple>();

static_assert(tuple_index_v<int, std::tuple<float, double>> == 2);



template <std::size_t Idx, typename ArgsTuple>
constexpr bool contained_in_exclusive = [] <std::size_t ... I>(std::index_sequence<I...>){
	return ([]<std::size_t Cur>(){
		return Idx != Cur && std::same_as<std::tuple_element_t<Idx, ArgsTuple>, std::tuple_element_t<Cur, ArgsTuple>>;
	}.template operator()<I>() || ...);
}(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>());

template <typename Tuple>
constexpr bool has_duplicate_types_impl = []<typename... Ts>(std::type_identity<std::tuple<Ts...>>){
	return [&]<std::size_t... Is>(std::index_sequence<Is...>){
		return ((contained_in_exclusive<Is, Tuple>) || ...);
	}(std::make_index_sequence<sizeof...(Ts) - 1>{});
}(std::type_identity<Tuple>{});

export
template <typename Tuple>
constexpr bool tuple_has_duplicate_types_v = []{
	if constexpr(std::tuple_size_v<Tuple> == 0){
		return false;
	} else{
		return has_duplicate_types_impl<Tuple>;
	}
}();


template <typename Tuple>
constexpr decltype(auto) createVariantFromTuple_Impl() noexcept{
	return [] <std::size_t ... I>(std::index_sequence<I...>){
		return std::variant<std::tuple_element_t<I, Tuple>...>{};
	}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

export
template <typename T>
using tuple_to_variant_t = decltype(createVariantFromTuple_Impl<T>());


#pragma region UNSTABLE_OffsetHack

//Tuple Offset Of

template <typename T>
struct alignas(T) type_replacement{
	std::byte _[sizeof(T)];
};


template <std::size_t I, typename Tuple>
consteval std::size_t element_offset() noexcept{
	using Tpl = unary_apply_to_tuple_t<type_replacement, Tuple>;
	union{
		char a[sizeof(Tpl)];
		Tpl t{};
	};
	auto* p = std::addressof(std::get<I>(t));
	t.~Tpl();
	for(std::size_t i = 0;; ++i){
		if(static_cast<void*>(a + i) == p) return i;
	}
}

export
template <std::size_t Index, typename Tuple>
inline constexpr std::size_t tuple_offset_at_v = element_offset<Index, Tuple>();

export
template <typename T, typename Tuple>
inline constexpr std::size_t tuple_offset_of_v = sizeof(Tuple) - sizeof(T) - element_offset<tuple_index_v<T, Tuple>, Tuple>();

#pragma endregion

#pragma region Legacy

template <std::size_t I, std::size_t size, typename ArgTuple, typename DefTuple>
constexpr decltype(auto) getWithDef(ArgTuple&& argTuple, DefTuple&& defTuple){
	if constexpr(I < size){
		return std::get<I>(std::forward<ArgTuple>(argTuple));
	} else{
		return std::get<I>(std::forward<DefTuple>(defTuple));
	}
}


template <typename TargetTuple, typename... Args, std::size_t... I>
constexpr decltype(auto) makeTuple_withDef_impl(
	std::tuple<Args...>&& args,
	TargetTuple&& defaults,
	std::index_sequence<I...>){
	return std::make_tuple(std::tuple_element_t<I, std::decay_t<TargetTuple>>{
			mo_yanxi::getWithDef<I, sizeof...(Args)>(std::move(args), std::forward<TargetTuple>(defaults))
		}...);
}


template <typename TargetTuple, typename FromTuple, std::size_t... I>
consteval bool tupleConvertableTo(std::index_sequence<I...>){
	return (std::convertible_to<std::tuple_element_t<I, FromTuple>, std::tuple_element_t<I, TargetTuple>> && ...);
}

template <typename TargetTuple, typename FromTuple, std::size_t... I>
consteval bool tupleSameAs(std::index_sequence<I...>){
	return (std::same_as<std::tuple_element_t<I, FromTuple>, std::tuple_element_t<I, TargetTuple>> && ...);
}


template <typename TargetTuple, typename T, std::size_t... I>
consteval bool tupleContains(std::index_sequence<I...>){
	return (std::same_as<T, std::tuple_element_t<I, TargetTuple>> || ...);
}

export
template <typename SuperTuple, typename... Args>
constexpr decltype(auto) make_tuple_with_def(SuperTuple&& defaultArgs, Args&&... args){
	return mo_yanxi::makeTuple_withDef_impl(std::make_tuple(std::forward<Args>(args)...),
		std::forward<SuperTuple>(defaultArgs),
		std::make_index_sequence<std::tuple_size_v<std::decay_t<SuperTuple>>>());
}

export
template <typename SuperTuple, typename... Args>
constexpr decltype(auto) make_tuple_with_def(Args&&... args){
	return mo_yanxi::make_tuple_with_def(SuperTuple{}, std::forward<Args>(args)...);
}

export
/**
 * @brief
 * @tparam strict Using std::same_as or std::convertable_to
 * @tparam SuperTuple Super sequence of the params
 * @tparam Args given params
 * @return Whether given param types are the subseq of the SuperTuple
 */
template <bool strict, typename SuperTuple, typename... Args>
constexpr bool is_types_sub_of(){
	constexpr std::size_t fromSize = sizeof...(Args);
	constexpr std::size_t toSize = std::tuple_size_v<SuperTuple>;
	if constexpr(std::tuple_size_v<SuperTuple> < fromSize) return false;

	if constexpr(strict){
		return mo_yanxi::tupleSameAs<SuperTuple, std::tuple<std::decay_t<Args>...>>(
			std::make_index_sequence<mo_yanxi::min_const<std::size_t, toSize, fromSize>>());
	} else{
		return mo_yanxi::tupleConvertableTo<SuperTuple, std::tuple<std::decay_t<Args>...>>(
			std::make_index_sequence<mo_yanxi::min_const<std::size_t, toSize, fromSize>>());
	}
}

export
template <typename T, typename ArgsTuple>
constexpr bool contained_in = requires{
	requires mo_yanxi::tupleContains<ArgsTuple, T>(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>());
};

export template <typename ToVerify, typename... Targets>
constexpr bool is_any_of = (std::same_as<ToVerify, Targets> || ...);

export
template <typename T, typename... Args>
constexpr bool contained_within = is_any_of<T, Args...>;

export
template <typename SuperTuple, typename FromTuple>
constexpr bool is_tuple_sub_of(){
	constexpr std::size_t fromSize = std::tuple_size_v<FromTuple>;
	constexpr std::size_t toSize = std::tuple_size_v<SuperTuple>;
	if constexpr(toSize < fromSize) return false;

	return mo_yanxi::tupleConvertableTo<SuperTuple, FromTuple>(
		std::make_index_sequence<mo_yanxi::min_const<std::size_t, toSize, fromSize>>());
}
#pragma endregion

#pragma region Tuple_Flatten

template <typename T>
struct to_ref_tuple{
	static consteval auto impl(T& t){
		return [&] <std::size_t ... I>(std::index_sequence<I...>){
			return std::make_tuple(std::ref(std::get<I>(t))...);
		}(std::make_index_sequence<std::tuple_size_v<T>>());
	}

	using type = decltype(to_ref_tuple::impl(std::declval<T&>()));
};

export
template <typename T>
using to_ref_tuple_t = typename to_ref_tuple<T>::type;

template <std::size_t idx, typename Ts>
struct flatten_tuple_impl{
	using type = std::tuple<Ts>;
	static constexpr std::size_t index = idx;
	static constexpr std::size_t stride = 1;

	using args = flatten_tuple_impl;

	template <std::size_t Idx, bool byRef = true, typename T>
	static constexpr decltype(auto) at(T&& t) noexcept{
		return std::forward<T>(t);
	}

	template <typename T>
	static constexpr T& forward_by_ref(T& t){
		return t;
	}

	template <typename T>
	static constexpr const T& forward_by_ref(const T& t){
		return t;
	}

	template <typename T>
	static constexpr T& forward_by_ref(T&& t) = delete;
};

template <std::size_t curIndex, typename... Ts>
struct flatten_tuple_impl<curIndex, std::tuple<Ts...>>{
	using type = decltype(std::tuple_cat(std::declval<typename flatten_tuple_impl<0, Ts>::type>()...));
	static constexpr std::size_t index = curIndex;
	static constexpr std::size_t stride = (flatten_tuple_impl<0, Ts>::stride + ...);

private:
	template <std::size_t ... I>
	static constexpr std::size_t stride_at(std::index_sequence<I...>){
		return (flatten_tuple_impl<0, std::tuple_element_t<I, std::tuple<Ts...>>>::stride + ... + 0);
	}

	static consteval auto unwrap_one(){
		return [&] <std::size_t ... I>(std::index_sequence<I...>){
			return
				std::tuple_cat(
					std::make_tuple(
						flatten_tuple_impl<flatten_tuple_impl::stride_at(std::make_index_sequence<I>()),
							std::tuple_element_t<
								I, std::tuple<Ts...>>>{}...),
					std::make_tuple(flatten_tuple_impl<stride, void>{}));
		}(std::index_sequence_for<Ts...>());
	}

public:
	using args = decltype(unwrap_one());

	template <std::size_t Idx, bool byRef = true, typename T>
		requires std::same_as<std::decay_t<T>, std::tuple<Ts...>>
	static constexpr decltype(auto) at(T&& t) noexcept{
		constexpr std::size_t validIndex = []<std::size_t ... I>(std::index_sequence<I...>){
			std::size_t rst{std::numeric_limits<std::size_t>::max()};
			((func<Idx, I>(rst)), ...);
			return rst;
		}(std::make_index_sequence<stride>());

		static_assert(validIndex != std::numeric_limits<std::size_t>::max(), "invalid index");

		using arg = std::tuple_element_t<validIndex, args>;
		return arg::template at<Idx - arg::index, byRef>(std::get<validIndex>(std::forward<T>(t)));
	}

private:
	template <std::size_t searchI, std::size_t intervalI>
	static constexpr bool is_index_valid = requires{
		requires searchI >= std::tuple_element_t<intervalI, args>::index;
		requires searchI < std::tuple_element_t<intervalI + 1, args>::index;
	};

	template <std::size_t searchI, std::size_t intervalI>
	static constexpr void func(std::size_t& rst){
		if constexpr(is_index_valid<searchI, intervalI>){
			rst = intervalI;
		}
	}
};

export
template <typename... Ts>
using flatten_tuple = flatten_tuple_impl<0, Ts...>;

export
template <typename... T>
using flatten_tuple_t = typename flatten_tuple<std::tuple<T...>>::type;


export
template <bool toRef, typename T>
constexpr std::conditional_t<toRef, to_ref_tuple_t<flatten_tuple_t<std::decay_t<T>>>, flatten_tuple_t<std::decay_t<T>>>
flat_tuple(T&& t) noexcept{
	using flatter = flatten_tuple<std::decay_t<T>>;
	if constexpr(toRef){
		return [&] <std::size_t ... I>(std::index_sequence<I...>){
			return std::make_tuple(std::ref(flatter::template at<I, toRef>(std::forward<T>(t)))...);
		}(std::make_index_sequence<flatter::stride>());
	} else{
		return [&] <std::size_t ... I>(std::index_sequence<I...>){
			return flatten_tuple_t<std::decay_t<T>>(flatter::template at<I, toRef>(std::forward<T>(t))...);
		}(std::make_index_sequence<flatter::stride>());
	}
}

export
template <bool toRef = false>
struct tuple_flatter{
	template <typename T>
	constexpr decltype(auto) operator()(T&& t) const noexcept{
		return mo_yanxi::flat_tuple<toRef>(std::forward<T>(t));
	}
};



#pragma endregion

#pragma region

#pragma endregion
}

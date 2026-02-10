module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.utility;

import mo_yanxi.meta_programming;
import std;

namespace mo_yanxi{

#pragma region Redundant_Construct

template <typename T, typename ...Args>
consteval bool is_constructible_from(std::type_identity<std::tuple<Args...>>){
	return std::constructible_from<T, Args...>;
}

template <typename T, typename ...Args>
consteval bool is_nothrow_constructible_from(std::type_identity<std::tuple<Args...>>){
	return std::is_nothrow_constructible_v<T, Args...>;
}

struct redundant_test_result{
	std::size_t drop;
	std::size_t drop_back;

	explicit consteval operator bool() const noexcept{
		return drop != std::dynamic_extent && drop_back != std::dynamic_extent;
	}
};

template <typename T, std::size_t skip_front, typename ...Args>
consteval redundant_test_result test_constructible_drop_BACK_MAJOR_param_v(){
    std::size_t rst_skip = std::dynamic_extent;
    std::size_t rst_drop = 0;

    using Tup = std::tuple<Args...>;
    static constexpr std::size_t argc = sizeof...(Args);
	static_assert(argc >= skip_front);
    static constexpr std::size_t back_size = argc - skip_front;

    using Front = tuple_take_n_elem_t<skip_front, Tup>;
    using Back = tuple_drop_front_n_elem_t<skip_front, Tup>;

    [&]<std::size_t ... Skip>(std::index_sequence<Skip...>){
       ([&]<std::size_t CurSkip>(){
          using BackCur = tuple_drop_front_n_elem_t<CurSkip, Back>;
          return [&]<std::size_t ... Drop>(std::index_sequence<Drop...>){
             return ([&]<std::size_t CurDrop>(){
                using ConstructParams = tuple_cat_t<Front, tuple_drop_back_n_elem_t<CurDrop, BackCur>>;

                if constexpr(mo_yanxi::is_constructible_from<T>(std::type_identity<ConstructParams>{})){
	                rst_skip = CurSkip;
	                rst_drop = CurDrop;
	                return true;
                }
                return false;
             }.template operator()<Drop>() || ...);
          }(std::make_index_sequence<std::tuple_size_v<BackCur>>{});
       }.template operator()<Skip>() || ...);
    }(std::make_index_sequence<back_size + 1>{});

    if (rst_skip == std::dynamic_extent) {
        using FinalParams = tuple_cat_t<Front, std::tuple<>>;

        if constexpr(mo_yanxi::is_constructible_from<T>(std::type_identity<FinalParams>{})){
           rst_skip = back_size;
           rst_drop = 0;
        }
    }

    return redundant_test_result{rst_skip, rst_drop};
}

template <typename T, std::size_t skip_front, typename ...Args>
consteval redundant_test_result test_constructible_drop_FRONT_MAJOR_param_v(){
	std::size_t rst_skip = std::dynamic_extent;
	std::size_t rst_drop = 0;

	using Tup = std::tuple<Args...>;
	static constexpr std::size_t argc = sizeof...(Args);
	static_assert(argc >= skip_front);
	static constexpr std::size_t back_size = argc - skip_front;

	using Front = tuple_take_n_elem_t<skip_front, Tup>;
	using Back = tuple_drop_front_n_elem_t<skip_front, Tup>;

	[&]<std::size_t ... Drop>(std::index_sequence<Drop...>){
		([&]<std::size_t CurDrop>(){
			using BackCur = tuple_drop_back_n_elem_t<CurDrop, Back>;

			return [&]<std::size_t ... Skip>(std::index_sequence<Skip...>){
			   return ([&]<std::size_t CurSkip>(){
				  using ConstructParams = tuple_cat_t<Front, tuple_drop_front_n_elem_t<CurSkip, BackCur>>;

				  if constexpr(mo_yanxi::is_constructible_from<T>(std::type_identity<ConstructParams>{})){
					  rst_skip = CurSkip;
					  rst_drop = CurDrop;
					  return true;
				  }
				  return false;
			   }.template operator()<Skip>() || ...);
			}(std::make_index_sequence<std::tuple_size_v<BackCur>>{});

		}.template operator()<Drop>() || ...);
	}(std::make_index_sequence<back_size + 1>{});

	if (rst_skip == std::dynamic_extent) {
		using FinalParams = tuple_cat_t<Front, std::tuple<>>;

		if constexpr(mo_yanxi::is_constructible_from<T>(std::type_identity<FinalParams>{})){
			rst_skip = back_size;
			rst_drop = 0;
		}
	}

	return redundant_test_result{rst_skip, rst_drop};
}

template <typename T, std::size_t skip_front, typename ...Args>
consteval std::size_t is_nothrow_back_redundant_constructible(){
	static constexpr redundant_test_result rst = test_constructible_drop_BACK_MAJOR_param_v<T, skip_front, Args...>();
	if constexpr (!rst)return true;

	using Tup = std::tuple<Args...>;
	using Front = tuple_take_n_elem_t<skip_front, Tup>;
	using Back = tuple_drop_front_n_elem_t<skip_front, Tup>;
	using BackDropped_1 = tuple_drop_front_n_elem_t<rst.drop, Back>;
	using BackDropped_2 = tuple_drop_back_n_elem_t<rst.drop_back, BackDropped_1>;
	using ConstructParams = tuple_cat_t<Front, BackDropped_2>;

	return mo_yanxi::is_nothrow_constructible_from<T>(std::type_identity<ConstructParams>{});
}

template <typename T, std::size_t skip_front, typename ...Args>
consteval std::size_t is_nothrow_front_redundant_constructible(){
	static constexpr redundant_test_result rst = test_constructible_drop_FRONT_MAJOR_param_v<T, skip_front, Args...>();
	if constexpr (!rst)return true;

	using Tup = std::tuple<Args...>;
	using Front = tuple_take_n_elem_t<skip_front, Tup>;
	using Back = tuple_drop_front_n_elem_t<skip_front, Tup>;
	using BackDropped_1 = tuple_drop_front_n_elem_t<rst.drop, Back>;
	using BackDropped_2 = tuple_drop_back_n_elem_t<rst.drop_back, BackDropped_1>;
	using ConstructParams = tuple_cat_t<Front, BackDropped_2>;

	return mo_yanxi::is_nothrow_constructible_from<T>(std::type_identity<ConstructParams>{});
}

export
template <typename T, std::size_t skip_front = 0, typename ...Args>
[[nodiscard]] constexpr T back_redundant_construct(Args&&... args) noexcept(is_nothrow_back_redundant_constructible<T, skip_front, Args&&...>()) {
	static constexpr redundant_test_result drops = test_constructible_drop_BACK_MAJOR_param_v<T, skip_front, Args&&...>();
	static_assert(drops, "Unable to construct T from args");

	auto fwd = std::forward_as_tuple(std::forward<Args>(args)...);

	return [&]<std::size_t ...Idx>(std::index_sequence<Idx...>){
		return T(std::get<Idx + (Idx >= skip_front ? drops.drop : 0uz)>(std::move(fwd)) ...);
	}(std::make_index_sequence<sizeof...(Args) - drops.drop - drops.drop_back>{});
}

export
template <typename T, std::size_t skip_front = 0, typename ...Args>
[[nodiscard]] constexpr T front_redundant_construct(Args&&... args) noexcept(is_nothrow_front_redundant_constructible<T, skip_front, Args&&...>()) {
	static constexpr redundant_test_result drops = test_constructible_drop_FRONT_MAJOR_param_v<T, skip_front, Args&&...>();
	static_assert(drops, "Unable to construct T from args");

	auto fwd = std::forward_as_tuple(std::forward<Args>(args)...);

	return [&]<std::size_t ...Idx>(std::index_sequence<Idx...>){
		return T(std::get<Idx + (Idx >= skip_front ? drops.drop : 0uz)>(std::move(fwd)) ...);
	}(std::make_index_sequence<sizeof...(Args) - drops.drop - drops.drop_back>{});
}

#pragma endregion

template <class Ty>
concept Boolean_testable_impl = std::convertible_to<Ty, bool>;

export
template <class Ty>
concept boolean_testable = Boolean_testable_impl<Ty> && requires(Ty&& t){
	{ !static_cast<Ty&&>(t) } -> Boolean_testable_impl;
};

export
template <typename T>
	requires requires(T& t){
		{ t.try_lock() } noexcept -> boolean_testable;
		{ t.unlock() } noexcept;
	}
FORCE_INLINE constexpr void assert_unlocked(T& mutex) noexcept{
#if MO_YANXI_UTILITY_ENABLE_CHECK
	if(mutex.try_lock()){
		mutex.unlock();
		return;
	}
	std::println(std::cerr, "Unlocked mutex assertion failed");
	std::terminate();
#endif
}

export
template <typename T>
	requires requires(T& t){
		{ t.try_lock() } noexcept -> boolean_testable;
	}
FORCE_INLINE constexpr void assert_locked(T& mutex) noexcept{
#if MO_YANXI_UTILITY_ENABLE_CHECK
	if(mutex.try_lock()){
		std::println(std::cerr, "Unlocked mutex assertion failed");
		std::terminate();
	}
#endif
}

export
template <typename... Fs>
struct overload : private Fs...{
	template <typename... Args>
	constexpr explicit(false) overload(Args&&... fs) : Fs{std::forward<Args>(fs)}...{
	}

	using Fs::operator()...;
};

export
template <typename... Fs>
overload(Fs&&...) -> overload<std::decay_t<Fs>...>;

export
template <typename... Fs>
struct overload_narrow : private Fs...{
	template <typename... Args>
	constexpr explicit(false) overload_narrow(Args&&... fs) : Fs{std::forward<Args>(fs)}...{
	}

	using Fs::operator()...;

	template <typename... Args>
	static void operator()(Args&&... args){
		(void)((void)args, ...);
		throw std::bad_variant_access{};
	}
};

export
template <typename... Fs>
overload_narrow(Fs&&...) -> overload_narrow<std::decay_t<Fs>...>;

export
template <typename Ret, typename... Fs>
	requires (std::is_void_v<Ret> || std::is_default_constructible_v<Ret>)
struct overload_def_noop : private Fs...{
	template <typename V, typename... Args>
	constexpr explicit(false) overload_def_noop(std::in_place_type_t<V>, Args&&... fs) : Fs{std::forward<Args>(fs)}...{
	}

	constexpr overload_def_noop() noexcept requires(sizeof...(Fs) == 0) = default;

	using Fs::operator()...;

	template <typename... T>
		requires (!std::invocable<Fs, T...> && ...)
	static Ret operator()(T&&...) noexcept{
		if constexpr(std::is_void_v<Ret>){
			return;
		} else{
			return Ret{};
		}
	}
};

export
template <typename V, typename... Fs>
overload_def_noop(std::in_place_type_t<V>, Fs&&...) -> overload_def_noop<V, std::decay_t<Fs>...>;


export
template <typename T, std::predicate<std::ranges::range_reference_t<T>> Pred>
constexpr void modifiable_erase_if(T& vec, Pred pred) noexcept(
	std::is_nothrow_invocable_v<Pred, T&> && std::is_nothrow_move_assignable_v<T>
){
	auto write_pos = vec.begin();
	auto read_pos = vec.begin();
	const auto end = vec.end();
	while(read_pos != end){
		if(!std::invoke(pred, *read_pos)){
			if(write_pos != read_pos){
				*write_pos = std::ranges::iter_move(read_pos);
			}
			++write_pos;
		}
		++read_pos;
	}
	vec.erase(write_pos, end);
}

export
template <class Rep, typename Period>
[[nodiscard]] auto to_absolute_time(const std::chrono::duration<Rep, Period>& rel_time) noexcept{
	constexpr auto zero = std::chrono::duration<Rep, Period>::zero();
	const auto cur = std::chrono::steady_clock::now();
	decltype(cur + rel_time) abs_time = cur; // return common type
	if(rel_time > zero){
		constexpr auto forever = (std::chrono::steady_clock::time_point::max)();
		if(abs_time < forever - rel_time){
			abs_time += rel_time;
		} else{
			abs_time = forever;
		}
	}
	return abs_time;
}

export
template <typename T>
constexpr auto pass_fn(T& fn) noexcept{
	if constexpr(sizeof(T) > sizeof(void*) * 2 || !std::constructible_from<T, const T&>){
		return std::ref(fn);
	} else{
		return fn;
	}
}


CONST_FN FORCE_INLINE constexpr std::strong_ordering connect_three_way_result(const std::strong_ordering cur) noexcept{
	return cur;
}

export
template <typename... T>
	requires (std::convertible_to<T, std::strong_ordering> && ...)
CONST_FN FORCE_INLINE constexpr std::strong_ordering connect_three_way_result(
	const std::strong_ordering cur, const T&... rst) noexcept{
	if(std::is_eq(cur)){
		return mo_yanxi::connect_three_way_result(rst...);
	} else{
		return cur;
	}
}


export
template <typename T>
	requires (std::integral<T> || std::is_enum_v<T>)
CONST_FN FORCE_INLINE constexpr std::strong_ordering compare_bitflags(const T lhs, const T rhs) noexcept{
	if constexpr(std::is_scoped_enum_v<T>){
		return mo_yanxi::compare_bitflags(std::to_underlying(lhs), std::to_underlying(rhs));
	} else{
		if(lhs == rhs) return std::strong_ordering::equal;
		if((lhs & rhs) == rhs) return std::strong_ordering::greater;
		return std::strong_ordering::less;
	}
}


export
struct dereference{
	template <std::indirectly_readable T>
	constexpr decltype(auto) operator()(T&& val) const noexcept{
		if constexpr(std::equality_comparable_with<T, std::nullptr_t>){
			assert(val != nullptr);
		}

		return *(std::forward<T>(val));
	}
};

namespace ranges::views{
export constexpr auto deref = std::views::transform(dereference{});
}


template <typename Fn, typename TupLike>
constexpr bool is_appliable_v = []{
	return []<std::size_t ... Idx>(std::index_sequence<Idx...>){
		return std::invocable<Fn, std::tuple_element_t<Idx, TupLike>...>;
	}(std::make_index_sequence<std::tuple_size_v<TupLike>>{});
}();

template <typename Fn, typename Tup>
struct apply_result;

template <typename Fn, typename... Args>
struct apply_result<Fn, std::tuple<Args...>> : std::type_identity<std::invoke_result_t<Fn, Args...>>{
};

template <typename Fn, typename Tup>
using apply_result_t = typename apply_result<Fn, Tup>::type;

template <typename Fn, typename TupLike>
void helper(TupLike&& tupLike){
	[&]<std::size_t ... Idx>(std::index_sequence<Idx...>){
		Fn::operator()(std::get<Idx>(std::forward<TupLike>(tupLike))...);
	}(std::make_index_sequence<std::tuple_size_v<TupLike>>{});
}

template <typename Fn, typename TupLike>
constexpr bool is_static_appliable_v = requires(TupLike&& tupLike){
#ifdef __clang__
	helper<Fn>(std::forward<TupLike>(tupLike));
#else
	std::apply(Fn::operator(), std::forward<TupLike>(tupLike));
#endif
};

template <typename T>
struct  is_noexcept_signature;

template <typename R, typename ...Args>
struct  is_noexcept_signature<R(Args...)> : std::false_type{};


template <typename R, typename ...Args>
struct  is_noexcept_signature<R(Args...) noexcept> : std::true_type{};


template <typename Fn>
constexpr inline bool is_noexcept_signature_v = is_noexcept_signature<Fn>::value;

constexpr bool b = is_noexcept_signature_v<void() noexcept>;

export
template <typename FnSignature, typename Fn>
auto func_take_any_params(Fn&& fn) noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<Fn>, Fn&&>){
	using namespace mo_yanxi;
	using fn_raw = std::remove_cvref_t<Fn>;
	using target_trait = function_traits<FnSignature>;
	using target_args = target_trait::mem_func_args_type;
	static constexpr auto sz = std::tuple_size_v<target_args>;

	static constexpr auto Idx = [] constexpr{
		std::size_t idx = -1;
		[&]<std::size_t ... Idx>(std::index_sequence<Idx...>){
			([&]<std::size_t I>{
				if constexpr(is_appliable_v<Fn, tuple_take_n_elem_t<sz - Idx, target_args>>){
					idx = I;
					return true;
				}
				return false;
			}.template operator()<Idx>() || ...);
		}(std::make_index_sequence<sz + 1>{});
		return idx;
	}();

	static_assert(Idx != -1, "Function Parameters Not Match");

	static constexpr auto seq = std::make_index_sequence<sz - Idx>{};
	static constexpr bool is_noexcept = []<std::size_t ... K>(std::index_sequence<K...>){
		return std::is_nothrow_invocable_v<Fn&&, std::tuple_element_t<K, target_args>&&...>;
	}(seq);

	return [&]<std::size_t ... J>(std::index_sequence<J...>){
		if constexpr(is_static_appliable_v<fn_raw, tuple_take_n_elem_t<sz - Idx, target_args>>){
			return [](std::tuple_element_t<J, target_args>... args) static noexcept(is_noexcept_signature_v<FnSignature>){
				auto argTup = std::forward_as_tuple(std::forward<decltype(args)>(args)...);
				using accpetables = tuple_take_n_elem_t<sz - Idx, decltype(argTup)>;
				if constexpr(std::convertible_to<
					apply_result_t<fn_raw, accpetables>, typename target_trait::return_type>){
					return [&]<std::size_t ... K>(std::index_sequence<K...>) noexcept(is_noexcept) -> typename target_trait::return_type{
						return fn_raw::operator()(std::forward<std::tuple_element_t<K, target_args>>(std::get<K>(argTup))...);
					}(seq);
				} else{
					[&]<std::size_t ... K>(std::index_sequence<K...>) noexcept(is_noexcept){
						fn_raw::operator()(std::forward<std::tuple_element_t<K, target_args>>(std::get<K>(argTup))...);
					}(seq);

					if constexpr(std::is_default_constructible_v<typename target_trait::return_type>){
						return typename target_trait::return_type{};
					} else if constexpr(std::is_void_v<typename target_trait::return_type>){
						return;
					} else{
						static_assert(false, "Cannot Match Return Type");
					}
				}
			};
		} else{
			return [f = std::forward<Fn>(fn)](std::tuple_element_t<J, target_args>... args) noexcept(is_noexcept_signature_v<FnSignature>){
				auto argTup = std::forward_as_tuple(std::forward<decltype(args)>(args)...);
				using accpetables = tuple_take_n_elem_t<sz - Idx, decltype(argTup)>;

				if constexpr(std::convertible_to<
					apply_result_t<fn_raw, accpetables>, typename target_trait::return_type>){
					return [&]<std::size_t ... K>(std::index_sequence<K...>) noexcept(is_noexcept){
						return std::invoke_r<typename target_trait::return_type>(
							f, std::forward<std::tuple_element_t<K, target_args>>(std::get<K>(argTup))...);
					}(seq);
				} else{
					[&]<std::size_t ... K>(std::index_sequence<K...>) noexcept(is_noexcept){
						std::invoke(f, std::forward<std::tuple_element_t<K, target_args>>(std::get<K>(argTup))...);
					}(seq);

					if constexpr(std::is_default_constructible_v<typename target_trait::return_type>){
						return typename target_trait::return_type{};
					} else if constexpr(std::is_void_v<typename target_trait::return_type>){
						return;
					} else{
						static_assert(false, "Cannot Match Return Type");
					}
				}
			};
		}
	}(std::make_index_sequence<sz>{});
}
}

//
// Created by Matrix on 2026/3/14.
//

export module mo_yanxi.cond_exist;

import std;

namespace mo_yanxi{

template <typename T>
void helper(T v){

}

template <typename T, typename ...Args>
concept impicit_construtible = requires(Args&& ...args){
	mo_yanxi::helper<T>({std::forward<Args>(args)...});
};


export
template <typename T, bool HasVal>
struct cond_exist{
	using value_type = T;

	template<typename ...Args>
		// requires std::constructible_from<T, Args&&...>
	explicit(false) cond_exist(Args&&... args) noexcept {}


	template <typename Fn, typename S, typename ...Args>
		// requires (std::invocable<Fn, S&&, Args&&...>)
	constexpr void invoke(this S&& self, Fn&& fn, Args&&... args) noexcept{

	}

	constexpr cond_exist& operator=(auto&&){
		return *this;
	}
};

template <typename T>
struct cond_exist<T, true>{
	using value_type = T;

	T val;

	constexpr cond_exist& operator=(value_type&& v) noexcept(std::is_nothrow_move_assignable_v<value_type>) requires(std::is_move_assignable_v<value_type>){
		val = std::move(v);
		return *this;
	}

	constexpr cond_exist& operator=(const value_type& v) noexcept(std::is_nothrow_copy_assignable_v<value_type>) requires(std::is_copy_assignable_v<value_type>){
		val = v;
		return *this;
	}

	template<typename ...Args>
		requires std::constructible_from<T, Args&&...>
	constexpr explicit(!impicit_construtible<T, Args&&...>) cond_exist(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) : val(std::forward<Args>(args)...){

	}

	template <typename Fn, typename S, typename ...Args>
		requires (std::invocable<Fn&&, S&&, Args&&...>)
	constexpr decltype(auto) invoke(this S&& self, Fn&& fn, Args&&... args) noexcept(std::is_nothrow_constructible_v<Fn&&, S&&, Args&&...>){
		return std::invoke(std::forward<Fn>(fn), std::forward<S>(self), std::forward<Args>(args)...);
	}

	constexpr T* operator->() noexcept {
		return std::addressof(val);
	}

	constexpr const T* operator->() const noexcept {
		return std::addressof(val);
	}

	constexpr T& operator*() noexcept {
		return val;
	}

	constexpr const T& operator*() const noexcept {
		return val;
	}

	constexpr explicit(false) operator T&() & noexcept{
		return val;
	}

	constexpr explicit(false) operator const T&() const & noexcept{
		return val;
	}

	constexpr explicit(false) operator T&&() && noexcept{
		return std::move(val);
	}

	constexpr explicit(false) operator const T&&() const && noexcept{
		return std::move(val);
	}

	template <typename S, typename Ty>
		requires requires(S&& self){
			static_cast<Ty>(std::forward_like<S>(self.val));
		}
	constexpr explicit(!std::convertible_to<S&&, Ty>) operator Ty(this S&& self) noexcept(noexcept(static_cast<Ty>(std::forward_like<S>(self.val)))){
		return static_cast<Ty>(std::forward_like<S>(self.val));
	}
};


}

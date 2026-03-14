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
		requires std::constructible_from<T, Args&&...>
	explicit(false) cond_exist(Args&&... args){}

};

template <typename T>
struct cond_exist<T, true>{
	using value_type = T;

	T val;

	template<typename ...Args>
		requires std::constructible_from<T, Args&&...>
	explicit(!impicit_construtible<T, Args&&...>) cond_exist(Args&&... args) : val(std::forward<Args>(args)...){

	}

	T* operator->() noexcept {
		return std::addressof(val);
	}

	const T* operator->() const noexcept {
		return std::addressof(val);
	}

	T& operator*() noexcept {
		return val;
	}

	const T& operator*() const noexcept {
		return val;
	}

	template <typename S>
	constexpr explicit(false) operator T&(this S&& self) noexcept{
		return std::forward_like<S>(self.val);
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

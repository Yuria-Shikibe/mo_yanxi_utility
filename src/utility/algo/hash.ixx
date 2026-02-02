module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.algo.hash;

import std;
import mo_yanxi.concepts;

namespace mo_yanxi::algo{
	//TODO power of two optimization

	template <typename Rng>
	consteval bool is_constexpr_range_with_power_of_2_size() noexcept{
		static constexpr auto b = is_statically_sized_range_size<Rng>::value;
		return std::has_single_bit(b);
	}

	export
	template <
	std::ranges::random_access_range Rng,
	std::forward_iterator It,
	std::sentinel_for<It> Se,
	std::indirectly_unary_invocable<It> Proj = std::identity,
	typename Hash = std::hash<std::remove_cvref_t<std::indirect_result_t<Proj, It>>>
	>
	requires requires{
		requires std::equality_comparable<std::indirect_result_t<Proj, It>>;
		requires std::is_copy_assignable_v<std::ranges::range_value_t<Rng>>;
	}
	constexpr void make_hash(
		Rng&& range,
		It begin,
		Se sentinel,
		Proj proj = {},
		const std::ranges::range_value_t<Rng>& empty = std::ranges::range_value_t<Rng>{},
		Hash hash = {}
		){

		const auto size = std::ranges::distance(range);
		if(size < std::ranges::distance(begin, sentinel)){
			throw std::out_of_range{"Insufficient Hashmap Size"};
		}

		std::ranges::fill(range, empty);

		for(; begin != sentinel; ++begin){
			auto pos = hash(std::invoke(proj, *begin)) % size;
			while(true){
				auto& tgt = range[pos];

				if(std::invoke(proj, tgt) == std::invoke(proj, empty)){
					tgt = *begin;
					break;
				}

				if(std::invoke(proj, tgt) == std::invoke(proj, *begin)){
					break;
				}

				pos = (pos + decltype(pos){1}) % size;
			}
		}
	}

	export
	template <
	std::ranges::random_access_range Rng,
	std::ranges::forward_range SourceRng,
	std::indirectly_unary_invocable<std::ranges::iterator_t<SourceRng>> Proj = std::identity,
	typename Hash = std::hash<std::remove_cvref_t<std::indirect_result_t<Proj, std::ranges::iterator_t<SourceRng>>>>

	>
	requires requires{
		requires std::equality_comparable<std::indirect_result_t<Proj, std::ranges::iterator_t<SourceRng>>>;
		requires std::is_copy_assignable_v<std::ranges::range_value_t<Rng>>;
	}
	constexpr void make_hash(
		Rng&& range,
		SourceRng&& src,
		Proj proj = {},
		const std::ranges::range_value_t<Rng>& empty = std::ranges::range_value_t<Rng>{},
		Hash hash = {}){

		algo::make_hash(
			std::forward<Rng>(range),
			std::ranges::begin(std::forward<SourceRng>(src)),
			std::ranges::end(std::forward<SourceRng>(src)), std::move(proj), empty, std::move(hash));
	}

	struct always_false{
		static bool operator()(auto&&) noexcept{
			return false;
		}
	};

	export
	template <
		std::ranges::random_access_range Rng,
		std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Proj = std::identity,
		typename Hash = std::hash<std::remove_cvref_t<std::indirect_result_t<Proj, std::ranges::iterator_t<Rng>>>>,
		std::predicate<const std::ranges::range_value_t<Rng>&> EmptyCheck = always_false
	>
	constexpr std::ranges::iterator_t<Rng> access_hash(
		Rng&& range,
		const std::remove_cvref_t<std::indirect_result_t<Proj, std::ranges::iterator_t<Rng>>>& value,
		Proj proj = {}, Hash hash = {}, EmptyCheck is_empty_pred = {}){

		const auto size = std::ranges::distance(range);
		static constexpr bool is_pow2 = is_constexpr_range_with_power_of_2_size<Rng&&>();
		static constexpr auto modder = [](auto index, auto sz) static constexpr noexcept {
			if constexpr (is_pow2){
				return index & (sz - 1);
			}else{
				return index % sz;
			}
		};

		const auto initial_pos = modder(hash(value), size);
		auto pos = initial_pos;

		do{
			auto cur = std::ranges::begin(range) + pos;

			if(value == std::invoke(proj, *cur)){
				return cur;
			}

			if(std::invoke(is_empty_pred, *cur)){
				return std::ranges::end(range);
			}

			pos = modder(pos + decltype(pos){1}, size);
		}while(pos != initial_pos);

		return std::ranges::end(range);
	}

	export
	template <
		std::ranges::random_access_range Rng,
		std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Proj = std::identity,
		typename Hash = std::hash<std::remove_cvref_t<std::indirect_result_t<Proj, std::ranges::iterator_t<Rng>>>>
	>
	constexpr std::ranges::iterator_t<Rng> access_hash(
		Rng&& range,
		const std::remove_cvref_t<std::indirect_result_t<Proj, std::ranges::iterator_t<Rng>>>& value,
		decltype(value) empty,
		Proj proj = {}, Hash hash = {}){
		const auto size = std::ranges::distance(range);

		static constexpr bool is_pow2 = is_constexpr_range_with_power_of_2_size<Rng&&>();
		static constexpr auto modder = [] FORCE_INLINE (auto index, auto sz) static constexpr noexcept {
			if constexpr (is_pow2){
				return index & (sz - 1);
			}else{
				return index % sz;
			}
		};

		const auto initial_pos = modder(hash(value), size);
		auto pos = initial_pos;

		do{
			auto cur = std::ranges::begin(range) + pos;
			decltype(auto) projed = std::invoke(proj, *cur);

			if(value == projed){
				return cur;
			}

			if(empty == projed){
				return std::ranges::end(range);
			}

			pos = modder(pos + decltype(pos){1}, size);
		}while(pos != initial_pos);

		return std::ranges::end(range);
	}

}
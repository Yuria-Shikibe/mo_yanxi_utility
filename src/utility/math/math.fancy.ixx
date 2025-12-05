module;

#include <cassert>

export module mo_yanxi.math.fancy;

import std;

namespace mo_yanxi::math{

	export
	template <std::input_iterator It, std::sentinel_for<It> Se,
		std::invocable<std::iter_difference_t<It>, float, float> LConsumer,
		std::invocable<std::iter_difference_t<It>, float, float> RConsumer>
	requires (std::convertible_to<std::iter_const_reference_t<It>, float>)
	constexpr void get_chunked_progress_region(It it, Se se, float cur, LConsumer lc, RConsumer rc){
		float last{};
		std::iter_difference_t<It> idx{};

		auto consume = [&](std::iter_difference_t<It> index, float next){
			if(cur > next){
				//(next, 1]
				lc(index, last, next);
			}else if(cur >= last){
				//[last, next)
				lc(index, last, cur);
				rc(index, cur, next);
			}else{
				rc(index, last, next);
				//[0, last)
			}
		};

		while(it != se){
			float next = *it;
			consume(idx, next);
			last = next;
			++it;
			++idx;
		}

		consume(idx, 1.f);
	}

	export
	template <std::input_iterator It, std::sentinel_for<It> Se,
		std::invocable<std::iter_difference_t<It>, float, float> Cons>
	requires (std::convertible_to<std::iter_const_reference_t<It>, float>)
	constexpr void get_chunked_progress_region(It it, Se se, Cons consumer){
		assert(std::ranges::distance(it, se) > 0);
		float last{*it};
		std::invoke(consumer, 0, 0, last);

		++it;
		std::iter_difference_t<It> idx{1};

		while(it != se){
			float next = *it;
			std::invoke(consumer, idx, last, next);
			last = next;
			++it;
			++idx;
		}

		std::invoke(consumer, idx, last, 1.f);
	}

	export
	template <std::ranges::input_range Rng = std::initializer_list<float>,
		std::invocable<std::ranges::range_difference_t<Rng>, float, float> LConsumer,
		std::invocable<std::ranges::range_difference_t<Rng>, float, float> RConsumer>
	requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, float>)
	constexpr void get_chunked_progress_region(Rng&& rng, float cur, LConsumer lc, RConsumer rc){
		math::get_chunked_progress_region(std::ranges::begin(std::as_const(rng)), std::ranges::end(std::as_const(rng)), cur, std::move(lc), std::move(rc));
	}

	export
	template <std::ranges::input_range Rng = std::initializer_list<float>,
		std::invocable<std::ranges::range_difference_t<Rng>, float, float> Consumer>
	requires (std::convertible_to<std::ranges::range_const_reference_t<Rng>, float>)
	constexpr void get_chunked_progress_region(Rng&& rng, Consumer consumer){
		math::get_chunked_progress_region(std::ranges::begin(std::as_const(rng)), std::ranges::end(std::as_const(rng)), std::move(consumer));
	}
}
module;

#include <cassert>

export module mo_yanxi.algo;
import mo_yanxi.concepts;
import std;


namespace mo_yanxi::algo{
	//heap search, fast, but not the best fit
	export
	template <
		std::random_access_iterator It,
		typename Pred = std::less<>,
		std::indirectly_unary_invocable<It> Proj = std::identity,
		typename ValTy>
		requires requires{
		requires std::predicate<Pred, std::indirect_result_t<Proj, It>, const ValTy&>;
		}
	It unstable_search_heap_upper_bound(It begin, const It end, const ValTy& val, const Pred pred = {}, const Proj proj = {}) noexcept {
		const auto stride = std::distance(begin, end);

		if(!stride)return end; //empty range

		auto current = begin;
		if(std::invoke(pred, std::invoke(proj, *current), val)){
			// val > proj(*current)
			return end;
		}

		while(true){
			const auto dst = std::distance(begin, current);

			if(const auto lchild_idx = dst * 2 + 1; lchild_idx < stride){
				const auto lchild = begin + lchild_idx;
				if(!std::invoke(pred, std::invoke(proj, *lchild), val)){
					//val <= *lchild -> !(val > *lchild)
					current = lchild;
					continue;
				}
			}

			if(const auto rchild_idx = dst * 2 + 2; rchild_idx < stride){
				const auto rchild = begin + rchild_idx;
				if(!std::invoke(pred, std::invoke(proj, *rchild), val)){
					current = rchild;
					continue;
				}
			}

			//val > both two elements and <= current, then current is the upper bound
			return current;
		}

		std::unreachable();
	}

	export
	template <
		std::ranges::random_access_range Rng,
		typename Pred = std::less<>,
		std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Proj = std::identity,
		typename ValTy>
		requires requires{
		requires std::predicate<Pred, std::indirect_result_t<Proj, std::ranges::iterator_t<Rng>>, const ValTy&>;
		}
	auto unstable_search_heap_upper_bound(Rng&& rng, const ValTy& val, const Pred pred = {}, const Proj proj = {}) noexcept {
		return algo::unstable_search_heap_upper_bound(std::ranges::begin(std::forward<Rng>(rng)), std::ranges::end(std::forward<Rng>(rng)), val, std::move(pred), std::move(proj));
	}
}

namespace mo_yanxi::algo{
	export
	template <
		std::input_iterator It,
		std::sentinel_for<It> Se,
		std::indirectly_unary_invocable<It> Proj = std::identity,
		typename Func = std::plus<>,
		typename T = std::indirect_result_t<Proj, It>>
		requires requires(Func func, T v, It it, Proj proj){
			{ std::invoke(func, std::move(v), std::invoke(proj, *it)) } -> std::convertible_to<T>;
		}
	constexpr T accumulate(It it, Se se, T initial = {}, Func func = {}, Proj proj = {}){
		for(; it != se; ++it){
			initial = std::invoke(func, std::move(initial), std::invoke(proj, *it));
		}

		return initial;
	}

	export
	template <
		typename Rng,
		typename Proj = std::identity, typename Func = std::plus<>,
		typename T = std::indirect_result_t<Proj, std::ranges::iterator_t<Rng>>>
	requires (!std::sentinel_for<T, Rng>)
	constexpr T accumulate(Rng&& rng, T initial, Func func = {}, Proj proj = {}){
		return mo_yanxi::algo::accumulate(std::ranges::begin(rng), std::ranges::end(rng), std::move(initial), std::move(func), std::move(proj));
	}

	export
	template <typename Range>
		requires mo_yanxi::range_of<Range>
	auto partBy(Range&& range, std::indirect_unary_predicate<std::ranges::iterator_t<Range>> auto pred){
		return std::make_pair(range | std::ranges::views::filter(pred),
			range | std::ranges::views::filter(std::not_fn(pred)));
	}

	template <typename Itr, typename Sentinel>
	constexpr bool itrSentinelCompariable = requires(Itr itr, Sentinel sentinel){
		{ itr < sentinel } -> std::convertible_to<bool>;
	};

	template <typename Itr, typename Sentinel>
	constexpr bool itrSentinelCheckable = requires(Itr itr, Sentinel sentinel){
		{ itr != sentinel } -> std::convertible_to<bool>;
	};

	template <typename Itr, typename Sentinel>
	constexpr bool itrRangeOrderCheckable = requires(Itr itr, Sentinel sentinel){
		{ itr <= sentinel } -> std::convertible_to<bool>;
	};

	template <typename Itr, typename Sentinel>
	constexpr void itrRangeOrderCheck(const Itr& itr, const Sentinel& sentinel){
		if constexpr(itrRangeOrderCheckable<Itr, Sentinel>){
			assert(itr <= sentinel);
		}
	}

	template <bool replace, typename Src, typename Dst>
	constexpr void swap_or_replace(Src src, Dst dst) noexcept(
		(replace && std::is_nothrow_move_assignable_v<std::iter_value_t<Src>>)
		||
		(!replace && std::is_nothrow_swappable_v<std::iter_value_t<Src>>)
		){
		if constexpr(replace){
			*dst = std::move(*src);
		} else{
			std::iter_swap(src, dst);
		}
	}


	template <
		bool replace,
		std::permutable Itr,
		std::permutable Sentinel,
		std::indirectly_unary_invocable<Itr> Proj = std::identity,
		std::indirect_unary_predicate<std::projected<Itr, Proj>> Pred
	>
	requires requires{
		requires std::bidirectional_iterator<Sentinel>;
		requires std::sentinel_for<Sentinel, Itr>;
	}
	[[nodiscard]] constexpr Itr
		remove_if_unstable_impl(Itr first, Sentinel sentinel, Pred pred, Proj porj = {}){
		algo::itrRangeOrderCheck(first, sentinel);

		first = std::ranges::find_if(first, sentinel, pred, porj);

		while(first != sentinel){
			--sentinel;
			algo::swap_or_replace<replace>(sentinel, first);

			first = std::ranges::find_if(first, sentinel, pred, porj);
		}

		return first;
	}

	template <bool replace, std::permutable Itr, std::permutable Sentinel, typename Ty, typename Proj = std::identity>
		requires requires{
		requires std::bidirectional_iterator<Sentinel>;
		requires std::sentinel_for<Sentinel, Itr>;
	}
	[[nodiscard]] constexpr Itr remove_unstable_impl(Itr first, Sentinel sentinel, const Ty& val, const Proj porj = {}){
		if constexpr(requires{
			algo::remove_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		}){
			return algo::remove_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		} else{
			return algo::remove_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<>{}, std::cref(val)), porj);
		}
	}

	template <bool replace, std::permutable Itr, std::permutable Sentinel, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<Itr, Proj>> Pred>
	requires requires(Sentinel sentinel){
		requires std::bidirectional_iterator<Sentinel>;
		requires std::sentinel_for<Sentinel, Itr>;
	}
	[[nodiscard]] constexpr Sentinel remove_unique_if_unstable_impl(Itr first, Sentinel sentinel, Pred pred, Proj porj = {}){
		algo::itrRangeOrderCheck(first, sentinel);

		Itr firstFound = std::ranges::find_if(first, sentinel, pred, porj);
		if(firstFound != sentinel){
			sentinel = std::prev(std::move(sentinel));

			algo::swap_or_replace<replace>(sentinel, firstFound);

			return sentinel;
		}

		return sentinel;
	}

	template <bool replace, std::permutable Itr, std::permutable Sentinel, typename Ty, typename Proj = std::identity>
	requires requires(Sentinel sentinel){--sentinel; requires std::sentinel_for<Sentinel, Itr>;}
	[[nodiscard]] constexpr Itr remove_unique_unstable_impl(Itr first, Sentinel sentinel, const Ty& val,
		const Proj porj = {}){

		if constexpr(requires{
			algo::remove_unique_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		}){
			return algo::remove_unique_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<Ty>{}, std::cref(val)), porj);
		} else{
			return algo::remove_unique_if_unstable_impl<replace>(first, sentinel, std::bind_front(std::equal_to<>{}, std::cref(val)), porj);
		}
	}

	export
	template <bool replace = false, std::permutable Itr, std::permutable Sentinel, typename Ty, typename Proj =
	          std::identity>
	[[nodiscard]] constexpr auto remove_unstable(Itr first, const Sentinel sentinel, const Ty& val,
		const Proj porj = {}){
		return std::ranges::subrange<Itr>{
				algo::remove_unstable_impl<replace>(first, sentinel, val, porj), sentinel
			};
	}

	export
	template <bool replace = false, std::ranges::random_access_range Rng, typename Ty, typename Proj = std::identity>
	[[nodiscard]] constexpr auto remove_unstable(Rng& range, const Ty& val, const Proj porj = {}){
		return std::ranges::borrowed_subrange_t<Rng>{
				algo::remove_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), val, porj),
				std::ranges::end(range)
			};
	}

	export
	template <bool replace = false, std::permutable Itr, std::permutable Sentinel, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<Itr, Proj>> Pred>
	[[nodiscard]] constexpr auto remove_if_unstable(Itr first, const Sentinel sentinel, Pred&& pred,
		const Proj porj = {}){
		return std::ranges::subrange<Itr>{
				algo::remove_if_unstable_impl<replace>(first, sentinel, pred, porj), sentinel
			};
	}

	export
	template <bool replace = false, std::ranges::random_access_range Rng, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Rng>, Proj>> Pred>
	[[nodiscard]] constexpr auto remove_if_unstable(Rng& range, Pred pred, const Proj porj = {}){
		return std::ranges::borrowed_subrange_t<Rng>{
				algo::remove_if_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), pred,
					porj),
				std::ranges::end(range)
			};
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Ty, typename Proj = std::identity>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;
			rng.erase(std::ranges::begin(rng), std::ranges::end(rng)); requires std::ranges::sized_range<Rng>;
		}
	constexpr decltype(auto) erase_unstable(Rng& range, const Ty& val, const Proj porj = {}){
		auto oldSize = range.size();
		range.erase(
			algo::remove_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), val, porj),
			std::ranges::end(range));
		return oldSize - range.size();
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Rng>, Proj>> Pred>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;
			rng.erase(std::ranges::begin(rng), std::ranges::end(rng));
		}
	constexpr decltype(auto) erase_if_unstable(Rng& range, const Pred pred, const Proj porj = {}){
		// auto oldSize = std::ranges::size(range);
		return range.erase(
			algo::remove_if_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), pred, porj),
			std::ranges::end(range));
		// return oldSize - std::ranges::size(range);
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Ty, typename Proj = std::identity>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;

			rng.erase(std::ranges::begin(rng), std::ranges::end(rng)); requires std::ranges::sized_range<Rng>;
		}
	constexpr decltype(auto) erase_unique_unstable(Rng& range, const Ty& val, const Proj porj = {}){
		auto oldSize = range.size();
		range.erase(
			algo::remove_unique_unstable_impl<replace>(std::ranges::begin(range), std::ranges::end(range), val,
				porj),
			std::ranges::end(range));
		return oldSize - range.size();
	}

	export
	template <bool replace = true, std::ranges::random_access_range Rng, typename Proj = std::identity,
	          std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<Rng>, Proj>> Pred>
		requires requires(Rng rng){
			requires std::ranges::sized_range<Rng>;
			rng.erase(std::ranges::begin(rng), std::ranges::end(rng)); requires std::ranges::sized_range<Rng>;
		}
	constexpr decltype(auto) erase_unique_if_unstable(Rng& range, Pred&& pred, const Proj porj = {}){
		auto oldSize = range.size();
		range.erase(
			algo::remove_unique_if_unstable_impl<
				replace>(std::ranges::begin(range), std::ranges::end(range), pred, porj), std::ranges::end(range));
		return oldSize - range.size();
	}

}

namespace mo_yanxi::algo{

}
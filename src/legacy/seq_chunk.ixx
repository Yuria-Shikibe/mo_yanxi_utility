//
// Created by Matrix on 2025/4/16.
//

export module mo_yanxi.seq_chunk;

import mo_yanxi.meta_programming;
import mo_yanxi.concepts;
import std;

namespace mo_yanxi{
	template <typename T>
	struct raw_wrap{
		T val;

		constexpr explicit(false) operator const T&() const{
			return val;
		}

		constexpr explicit(false) operator T&(){
			return val;
		}

		friend constexpr bool operator==(const raw_wrap&, const raw_wrap&) noexcept = default;
	};

	template <typename T, typename Tuple>
	constexpr decltype(auto) try_get(Tuple&& tuple) noexcept {
		if constexpr (tuple_index_v<T, std::remove_cvref_t<Tuple>> == std::tuple_size_v<std::remove_cvref_t<Tuple>>){
			return T{};
		}else{
			return std::forward_like<Tuple>(std::get<T>(tuple));
		}
	}

	template <typename T>
	using chunk_partial = std::conditional_t<std::is_fundamental_v<T>, raw_wrap<T>, T>;

	export
	template <typename... T>
	struct seq_chunk final : public chunk_partial<T>...{
		template <typename Ty>
			requires (contained_within<std::remove_cvref_t<Ty>, T...>)
		constexpr static copy_qualifier_t<Ty, seq_chunk>* chunk_cast(Ty* p) noexcept{
			return static_cast<copy_qualifier_t<Ty, seq_chunk>*>(p);
		}

		static constexpr auto chunk_element_count = sizeof...(T);
		using tuple_type = std::tuple<T...>;

		[[nodiscard]] constexpr seq_chunk() noexcept = default;

		template <typename ...Args>
			requires (sizeof...(Args) == sizeof...(T))
		[[nodiscard]] constexpr explicit(false) seq_chunk(Args&& ...args)
		: chunk_partial<T>{std::forward<Args>(args)} ...{

		}

		template <typename Tuple>
			requires (is_tuple_v<Tuple>)
		[[nodiscard]] constexpr explicit(false) seq_chunk(Tuple&& args)
		: chunk_partial<T>{mo_yanxi::try_get<T>(std::forward<Tuple>(args))} ...{

		}

		template <typename ...Args>
			requires (sizeof...(Args) < sizeof...(T) && sizeof...(Args) > 0)
		[[nodiscard]] constexpr explicit(sizeof...(Args) == 1) seq_chunk(Args&& ...args)
		: seq_chunk{std::make_tuple(std::forward<Args>(args)...)}{

		}

		template <typename S, typename C, typename Ty>
		constexpr Ty& operator->*(this S&& self, Ty C::* mptr) noexcept{
			return static_cast<copy_qualifier_t<S&&, C>>(std::forward<S>(self)).*mptr;
		}

		template <typename S, typename Fn>
			requires std::is_member_function_pointer_v<Fn>
		constexpr auto operator->*(this S&& self, Fn mfptr) noexcept{
			using trait = mptr_info<Fn>;
			using class_type = typename trait::class_type;

			return [&]<std::size_t ...Idx>(std::index_sequence<Idx...>){
				return [mfptr, &self](std::tuple_element_t<Idx, typename trait::func_args> ...args) -> decltype(auto) {
					return std::invoke(mfptr, static_cast<copy_qualifier_t<S&&, class_type>>(self), std::forward<decltype(args)>(args)...);
				};
			}(std::make_index_sequence<std::tuple_size_v<typename trait::func_args>>{});
		}

		template <typename S, typename Fn, typename ...Args>
			requires std::invocable<Fn, S, Args...>
		constexpr decltype(auto) invoke(this S&& self, Fn fn, Args&& ...args) noexcept(std::is_nothrow_invocable_v<Fn, S, Args...>){
			return std::invoke(fn, std::forward<S>(self), std::forward<Args>(args)...);
		}

		template <typename Ty, typename S>
		constexpr decltype(std::forward_like<S>(std::declval<Ty&>())) get(this S&& self) noexcept{
			return std::forward_like<S>(self);
		}

		template <std::size_t Idx, typename S>
		constexpr decltype(auto) get(this S&& self) noexcept{
			return std::forward<S>(self).template get<std::tuple_element_t<Idx, tuple_type>>();
		}

		// template <std::size_t Idx, typename Chunk>
		// friend constexpr decltype(auto) get(Chunk&& chunk) noexcept{
		// 	return chunk.template get<Idx>();
		// }

	};

	template <typename T>
	consteval auto ttsc(){
		return []<std::size_t ... I>(std::index_sequence<I...>){
			return std::type_identity<seq_chunk<std::tuple_element_t<I, T> ...>>{};
		}(std::make_index_sequence<std::tuple_size_v<T>>());
	}

	export
	template <typename Tuple>
	using tuple_to_seq_chunk_t = typename decltype(ttsc<Tuple>())::type;


	export
	template <spec_of<seq_chunk> Chunk, typename Ty>
	constexpr copy_qualifier_t<Ty, Chunk>* seq_chunk_cast(Ty* p) noexcept{
		return Chunk::chunk_cast(p);
	}

	export
	template <typename Tgt, spec_of<seq_chunk> ChunkTy, typename ChunkPartial>
		requires requires{
		requires contained_in<Tgt, typename ChunkTy::tuple_type>;
		requires contained_in<std::remove_cvref_t<ChunkPartial>, typename ChunkTy::tuple_type>;
		}
	[[nodiscard]] constexpr decltype(auto) neighbour_of(ChunkPartial& value) noexcept{
		using CTy = copy_qualifier_t<ChunkPartial, std::remove_cvref_t<ChunkTy>>;
		CTy* chunk = CTy::chunk_cast(std::addressof(value));
		return chunk->template get<Tgt>();//std::forward_like<ChunkPartial>();
	}

	export
	template <typename Tgt, tuple_spec ChunkTy, typename ChunkPartial>
		requires requires{
		requires contained_in<Tgt, ChunkTy>;
		requires contained_in<std::remove_cvref_t<ChunkPartial>, ChunkTy>;
		}
	[[nodiscard]] constexpr decltype(auto) neighbour_of(ChunkPartial& value) noexcept{
		using CTy = copy_qualifier_t<ChunkPartial, tuple_to_seq_chunk_t<std::remove_cvref_t<ChunkTy>>>;
		CTy* chunk = CTy::chunk_cast(std::addressof(value));
		return chunk->template get<Tgt>();//std::forward_like<ChunkPartial>();
	}


	/*export
	template <std::size_t N, typename Chunk>
		requires (N < std::tuple_size_v<typename Chunk::tuple_type>)
	using seq_chunk_truncate_t = typename decltype([]{
		if constexpr(N == 0){
			std::type_identity<seq_chunk<>>{};
		} else{
			return []<std::size_t ... I>(std::index_sequence<I...>){
				return std::type_identity<seq_chunk<std::tuple_element_t<I, typename Chunk::tuple_type>...>>{};
			}(std::make_index_sequence<N>());
		}
	}())::type;*/

	// export
	// template <std::size_t Idx, typename Chunk>
	// inline constexpr std::size_t seq_chunk_offset_at = tuple_offset_at_v<std::tuple_size_v<typename Chunk::tuple_type> - 1 - Idx, reverse_tuple_t<typename Chunk::tuple_type>>;;
	//
	// export
	// template <typename V, typename Chunk>
	// inline constexpr std::size_t seq_chunk_offset_of = seq_chunk_offset_at<tuple_index_v<V, typename Chunk::tuple_type>, Chunk>;


	export
	template <typename T, mo_yanxi::decayed_spec_of<mo_yanxi::seq_chunk> Chunk>
	constexpr decltype(auto) get(Chunk&& chunk) noexcept{
		return chunk.template get<T>();
	}

	export
	template <std::size_t Idx, mo_yanxi::decayed_spec_of<mo_yanxi::seq_chunk> Chunk>
	constexpr decltype(auto) get(Chunk&& chunk) noexcept{
		return chunk.template get<Idx>();
	}

}


template <typename ...T>
struct std::tuple_size<mo_yanxi::seq_chunk<T...>> : std::integral_constant<std::size_t, sizeof...(T)>{};

template <std::size_t Idx, typename ...T>
struct std::tuple_element<Idx, mo_yanxi::seq_chunk<T...>> : std::tuple_element<Idx, std::tuple<T...>>{};

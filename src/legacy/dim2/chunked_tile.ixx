module;

#include "mo_yanxi/adapted_attributes.hpp"
#include <cassert>

export module mo_yanxi.dim2.grid;

export import mo_yanxi.utility;
export import ext.dim2.plane_concept;
export import mo_yanxi.math.vector2;
export import mo_yanxi.dim2.tile;
export import mo_yanxi.open_addr_hash_map;
import std;

namespace mo_yanxi::dim2{
	template <typename T>
	using vec = math::vector2<T>;

	constexpr std::size_t ChunkWidth = 32;
	constexpr std::size_t ChunkHeight = 32;

	export
	template <std::signed_integral T = int>
	constexpr std::array<vec<T>, 4> proximate_4{
		vec<T>{static_cast<T>( 0), static_cast<T>( 1)},
		vec<T>{static_cast<T>( 1), static_cast<T>( 0)},
		vec<T>{static_cast<T>(-1), static_cast<T>( 0)},
		vec<T>{static_cast<T>( 0), static_cast<T>(-1)},
	};

	export
	template <std::signed_integral T = int>
	constexpr std::array<vec<T>, 8> proximate_8{
		vec<T>{static_cast<T>( 0), static_cast<T>( 1)},
		vec<T>{static_cast<T>( 1), static_cast<T>( 1)},
		vec<T>{static_cast<T>( 1), static_cast<T>( 0)},
		vec<T>{static_cast<T>(-1), static_cast<T>( 0)},
		vec<T>{static_cast<T>(-1), static_cast<T>( 0)},
		vec<T>{static_cast<T>(-1), static_cast<T>(-1)},
		vec<T>{static_cast<T>( 0), static_cast<T>(-1)},
		vec<T>{static_cast<T>( 1), static_cast<T>(-1)},
	};

	constexpr auto mod_to_positive(const std::integral auto lhs, const std::integral auto rhs){
		const auto r = lhs % rhs;
		if(r < 0){
			return r + rhs;
		}
		return r;
	}

	template <std::signed_integral T>
	constexpr auto div_to_positive(const T lhs, const T rhs){
		auto rst = std::div(lhs, rhs);
		if(rst.rem < 0){
			rst.rem += rhs;
			rst.quot -= 1;
		}
		return rst;
	}

	template <std::integral T1, std::integral T2>
	constexpr auto div_to_positive(const T1 lhs, const T2 rhs){
		using common_type = std::make_signed_t<std::common_type_t<T1, T2>>;
		auto rst = std::div(static_cast<common_type>(lhs), static_cast<common_type>(rhs));
		if(rst.rem < 0){
			rst.rem += rhs;
			rst.quot -= 1;
		}
		return rst;
	}

	struct div_result{

	};

	template <
		std::integral T1, std::integral T2, position_acquireable<T1> P1, position_acquireable<T2> P2>
	constexpr auto div_to_positive(const P1& lhs, const P2& rhs){
		using common_type = std::common_type_t<decltype(lhs.x), decltype(rhs.x)>;

		const auto rX = dim2::div_to_positive(lhs.x, rhs.x);
		const auto rY = dim2::div_to_positive(lhs.y, rhs.y);

		assert(rX.rem >= 0);
		assert(rY.rem >= 0);

		using UT = std::make_unsigned_t<common_type>;

		return std::make_pair(
			vec<std::make_signed_t<common_type>>{rX.quot, rY.quot},
			vec<UT>{static_cast<UT>(rX.rem), static_cast<UT>(rY.rem)});
	}

	// using T = int;
	// using SizeType = unsigned;

	export 
	template <typename T, typename CoordT>
	concept pos_acceptor = requires(T& t){
		t.set_pos(CoordT{});
	};
	
	export
	template <typename T, typename SizeType = unsigned, vec<SizeType> Extent = {ChunkWidth, ChunkHeight}>
		requires (Extent.area() > 0)
	struct chunk : tile_adaptor<chunk<T, SizeType, Extent>, T, SizeType>{
	private:
		using base = tile_adaptor<chunk, T, SizeType>;
		friend base;

	public:
		using size_type = std::make_unsigned_t<SizeType>;
		using global_size_type = std::make_signed_t<SizeType>;
		using difference_type = std::ptrdiff_t;

		using local_coord_type = vec<size_type>;
		using global_coord_type = vec<global_size_type>;
		static constexpr local_coord_type extent{Extent};
		static constexpr std::size_t chunk_capacity = extent.area();


		static_assert(std::numeric_limits<size_type>::max() >= extent.x);
		static_assert(std::numeric_limits<size_type>::max() >= extent.y);

		using value_type = T;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using container_type = std::array<value_type, extent.area()>;
	private:
		global_coord_type chunk_coord_{};

	public:
		container_type tiles{};

		[[nodiscard]] constexpr chunk() = default;

		[[nodiscard]] constexpr explicit chunk(typename global_coord_type::const_pass_t coord)
			: chunk_coord_{coord}{
			if constexpr (pos_acceptor<value_type, global_coord_type>){
				this->each([coord] <pos_acceptor<global_coord_type> Ty> (Ty& tile, size_type x, size_type y){
					tile.set_pos(global_coord_type{
						static_cast<global_size_type>(x + coord.x * extent.x),
						static_cast<global_size_type>(y + coord.y * extent.y)});
				});
			}
		}

		[[nodiscard]] constexpr global_coord_type to_global(const typename local_coord_type::const_pass_t local_coord) const noexcept{
			return local_coord.template as<global_size_type>() + chunk_coord_ * extent.template as<global_size_type>();
		}

		[[nodiscard]] static constexpr local_coord_type to_local(const typename global_coord_type::const_pass_t global_coord) noexcept{
			return local_coord_type{dim2::mod_to_positive(global_coord.x, extent.x), dim2::mod_to_positive(global_coord.y, extent.y)};
		}

		[[nodiscard]] static constexpr size_type width() noexcept{
			return extent.x;
		}

		[[nodiscard]] static constexpr size_type height() noexcept{
			return extent.y;
		}

		[[nodiscard]] constexpr global_coord_type chunk_coord() const noexcept{
			return chunk_coord_;
		}

		[[nodiscard]] constexpr global_coord_type global_src_coord() const noexcept{
			return chunk_coord_ * extent.template as<global_size_type>();
		}

		[[nodiscard]] constexpr global_coord_type global_end_coord() const noexcept{
			return chunk_coord_.copy().add(1, 1) * extent.template as<global_size_type>();
		}

	private:
		[[nodiscard]] constexpr const value_type* data_impl() const noexcept{
			return tiles.data();
		}

		[[nodiscard]] constexpr value_type* data_impl() noexcept{
			return tiles.data();
		}
	};

	export
	template <typename T, typename SizeType = unsigned, vec<SizeType> Extent = {ChunkWidth, ChunkHeight}>
	struct grid{
		using value_type = chunk<T, SizeType, Extent>;
		using size_type = std::make_unsigned_t<SizeType>;
		using global_size_type = std::make_signed_t<SizeType>;
		using difference_type = std::ptrdiff_t;

		using tile_type = T;

		using local_coord_type = vec<size_type>;
		using global_coord_type = vec<global_size_type>;

		// fixed_open_addr_hash_map<global_coord_type, value_type, global_coord_type, math::vectors::constant2<global_size_type>::lowest_vec2> chunks{};
		std::unordered_map<global_coord_type, value_type> chunks{};

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		[[nodiscard]] constexpr decltype(auto) tile_at(const Coord global_tile_coord){
			const auto [global, local] = dim2::div_to_positive<global_size_type, size_type>(global_tile_coord, value_type::extent);
			return chunks.at(global).at(local);
		}

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		[[nodiscard]] constexpr decltype(auto) tile_at(const Coord global_tile_coord) const {
			const auto [global, local] = dim2::div_to_positive<global_size_type, size_type>(global_tile_coord, value_type::extent);
			return chunks.at(global).at(local);
		}

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		[[nodiscard]] constexpr decltype(auto) chunk_at(const Coord chunk_coord){
			return chunks.at(chunk_coord);
		}

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		[[nodiscard]] constexpr decltype(auto) chunk_at(const Coord chunk_coord) const {
			return chunks.at(chunk_coord);
		}

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		constexpr decltype(auto) make_chunk(const Coord chunk_coord){
			return chunks.try_emplace(chunk_coord, chunk_coord);
		}

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		[[nodiscard]] constexpr decltype(auto) operator[](const Coord global_tile_coord){
			const auto [global, local] = dim2::div_to_positive<global_size_type, size_type>(global_tile_coord, value_type::extent);
			const auto [itr, rst] = chunks.try_emplace(global, global);
			return itr->second.at(local);
		}

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		[[nodiscard]] constexpr tile_type* find(const Coord global_tile_coord){
			const auto [global, local] = dim2::div_to_positive<global_size_type, size_type>(global_tile_coord, value_type::extent);
			if(auto itr = chunks.find(global); itr != chunks.end()){
				return &itr->second.at(local);
			}

			return nullptr;
		}

		template <position_acquireable<global_size_type> Coord = global_coord_type>
		[[nodiscard]] constexpr const tile_type* find(const Coord global_tile_coord) const {
			const auto [global, local] = dim2::div_to_positive<global_size_type, size_type>(global_tile_coord, value_type::extent);
			if(auto itr = chunks.find(global); itr != chunks.end()){
				return &itr->second.at(local);
			}

			return nullptr;
		}

		auto begin() noexcept{
			return chunks.begin();
		}

		auto end() noexcept{
			return chunks.end();
		}

		auto begin() const noexcept{
			return chunks.begin();
		}

		auto end() const noexcept{
			return chunks.end();
		}

		auto cbegin() noexcept{
			return chunks.cbegin();
		}

		auto cend() noexcept{
			return chunks.cend();
		}

		[[nodiscard]] constexpr bool empty() const noexcept{
			return chunks.empty();
		}


		template <typename Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<value_type>, Proj>> Pred>
		tile_type* find(Pred pred, Proj proj = {}){
			for (auto&& chunk : *this){
				if(auto itr = std::ranges::find_if(chunk.second, mo_yanxi::pass_fn(pred), mo_yanxi::pass_fn(proj)); itr != chunk.second.end()){
					return std::to_address(itr);
				}
			}
			return nullptr;
		}

		template <typename Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::const_iterator_t<value_type>, Proj>> Pred>
		const tile_type* find(Pred pred, Proj proj = {}) const{
			return const_cast<grid*>(this)->find(mo_yanxi::pass_fn(pred), mo_yanxi::pass_fn(proj));
		}
	};
}

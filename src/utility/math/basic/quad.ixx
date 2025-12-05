module;

#include "mo_yanxi/adapted_attributes.hpp"
#ifdef __AVX2__
#define ENABLE_SIMD
#include <immintrin.h>
#endif


export module mo_yanxi.math.quad;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
export import mo_yanxi.math.trans2;
export import mo_yanxi.math.intersection;
export import mo_yanxi.math;

import std;

namespace mo_yanxi::math{
#ifdef ENABLE_SIMD
	template<bool IsMin>
	FORCE_INLINE float horizontal_extreme(__m128 v) noexcept {
		__m128 v1 = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2,3,0,1)); // swap 0-1, 2-3
		__m128 m1 = IsMin ? _mm_min_ps(v, v1) : _mm_max_ps(v, v1);
		__m128 m2 = _mm_shuffle_ps(m1, m1, _MM_SHUFFLE(1,0,3,2)); // swap pairs
		__m128 res = IsMin ? _mm_min_ps(m1, m2) : _mm_max_ps(m1, m2);
		return _mm_cvtss_f32(_mm_shuffle_ps(res, res, _MM_SHUFFLE(0,0,0,0)));
	}
#endif


	template <typename T = unsigned>
	constexpr inline T vertex_mask{0b11};
	template <typename D, typename Base>
	concept any_derived_from = std::is_base_of_v<Base, D>;

	template <typename T>
	struct overlay_axis_keys{
		static constexpr std::size_t count = 0;

		template <std::size_t Idx>
		static constexpr vec2 get(const T&) noexcept = delete;
	};

	template <typename T>
	struct overlay_axis_keys<rect_ortho<T>>{
		static constexpr std::size_t count = 2;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const rect_ortho<T>&) noexcept{
			if constexpr (Idx == 0){
				return vectors::constant2<T>::x_vec2;
			}else{
				return vectors::constant2<T>::y_vec2;
			}
		}
	};

	template <std::size_t Idx, typename L, typename R>
	FORCE_INLINE constexpr vec2 get_axis(const L& l, const R& r) noexcept {
		using LK = overlay_axis_keys<L>;
		using RK = overlay_axis_keys<R>;

		static_assert(LK::count + RK::count >= Idx, "Idx Out Of Bound");

		auto a = Idx;

		if constexpr (Idx < LK::count){
			return LK::template get<Idx>(l);
		}else{
			return RK::template get<Idx - LK::count>(r);
		}
	}
	//
	template <typename L, typename R>
	FORCE_INLINE constexpr vec2 get_axis(std::size_t Idx, const L& l, const R& r) noexcept {
		using LK = overlay_axis_keys<L>;
		using RK = overlay_axis_keys<R>;

		if (Idx < LK::count){
			return l.edge_normal_at(Idx);
		}else{
			return r.edge_normal_at(Idx - LK::count);
		}
	}


	/**
	 * \brief Mainly Used For Continous Collidsion Test
	 * @code
	 * v3 +-----+ v2
	 *    |     |
	 *    |     |
	 * v0 +-----+ v1
	 * @endcode
	 */
	export
	template <typename T>
	struct quad{
		using value_type = T;
		using vec_t = vector2<value_type>;
		using rect_t = rect_ortho<value_type>;

		vec_t v0; //v00
		vec_t v1; //v10
		vec_t v2; //v11
		vec_t v3; //v01

		template <std::integral Idx>
		FORCE_INLINE constexpr vec_t& operator[](const Idx idx) noexcept{
			switch (idx & vertex_mask<Idx>){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		template <std::integral Idx>
		FORCE_INLINE constexpr vec_t operator[](const Idx idx) const noexcept{
			switch (idx & vertex_mask<Idx>){
				case 0: return v0;
				case 1: return v1;
				case 2: return v2;
				case 3: return v3;
				default: std::unreachable();
			}
		}

		constexpr friend bool operator==(const quad& lhs, const quad& rhs) noexcept = default;

		FORCE_INLINE constexpr void move(vec_t::const_pass_t vec) noexcept{
#ifdef ENABLE_SIMD
			if constexpr (std::same_as<float, value_type>){
				if (!std::is_constant_evaluated()){
					/* 创建重复的vec.x/vec.y模式 */
					const __m256 vec_x = ::_mm256_broadcast_ss(&vec.x); // [x,x,x,x,x,x,x,x]
					const __m256 vec_y = ::_mm256_broadcast_ss(&vec.y); // [y,y,y,y,y,y,y,y]

					// 通过解包操作生成[x,y,x,y,x,y,x,y]模式
					const __m256 vec_pattern = _mm256_unpacklo_ps(vec_x, vec_y);

					/* 一次性加载全部4个vec_t（8个float） */
					auto base_ptr = reinterpret_cast<float*>(this);
					__m256 data = _mm256_loadu_ps(base_ptr);

					/* 执行向量加法 */
					data = _mm256_add_ps(data, vec_pattern);

					/* 存回内存 */
					_mm256_storeu_ps(base_ptr, data);
					return;
				}
			}
#endif

			v0 += vec;
			v1 += vec;
			v2 += vec;
			v3 += vec;
		}

		[[nodiscard]] FORCE_INLINE constexpr rect_t get_bound() const noexcept{
			auto [xmin, xmax] = math::minmax(v0.x, v1.x, v2.x, v3.x);
			auto [ymin, ymax] = math::minmax(v0.y, v1.y, v2.y, v3.y);
			return rect_ortho<T>{tags::unchecked, tags::from_vertex, vec_t{xmin, ymin}, vec_t{xmax, ymax}};
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t avg() const noexcept{
			return (v0 + v1 + v2 + v3) / static_cast<value_type>(4);
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr bool overlap_rough(this const S& self, const O& o) noexcept{
			return self.get_bound().overlap_inclusive(o.get_bound());
		}

		template <any_derived_from<quad> S>
		[[nodiscard]] FORCE_INLINE constexpr bool overlap_rough(this const S& self, const rect_t& other) noexcept{
			return self.get_bound().overlap_inclusive(other);
		}

		template <typename S, typename O>
		[[nodiscard]] FORCE_INLINE constexpr bool overlap(this const S& self, const O& other) noexcept{
			return self.overlap_rough(other) && self.overlap_exact(other);
		}

		template <typename S, any_derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr bool contains(this const S& self, const O& other) noexcept{
			if(!self.get_bound().contains(other.get_bound()))return false;

			for(int i = 0; i < 4; ++i){
				if(!self.contains(other[i]))return false;
			}
			return true;
		}
		template <typename S>
		[[nodiscard]] FORCE_INLINE constexpr bool contains(this const S& self, const rect_t& other) noexcept{
			if(!self.get_bound().contains(other))return false;

			for(int i = 0; i < 4; ++i){
				if(!self.contains(other[i]))return false;
			}
			return true;
		}

		constexpr vec_t expand(const T length) noexcept{
			auto center = avg();

			v0 += (v0 - center).set_length(length);
			v1 += (v1 - center).set_length(length);
			v2 += (v2 - center).set_length(length);
			v3 += (v3 - center).set_length(length);

			return center;
		}

		/**
		 * @return normal on edge[idx, idx + 1], pointing to outside
		 */
		template <any_derived_from<quad> S, std::integral Idx>
		[[nodiscard]] FORCE_INLINE constexpr vec_t edge_normal_at(this const S& self, const Idx edgeIndex) noexcept{
			const auto begin = self[edgeIndex];
			const auto end   = self[edgeIndex + 1];

			return static_cast<vec_t&>((end - begin).rotate_rt_clockwise()).normalize();
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
		[[nodiscard]] FORCE_INLINE constexpr value_type axis_proj_dst(this const S& sbj, const O& obj, const vec_t axis) noexcept{
			const auto [min1, max1] = math::minmax(sbj[0].dot(axis), sbj[1].dot(axis), sbj[2].dot(axis), sbj[3].dot(axis));
			const auto [min2, max2] = math::minmax(obj[0].dot(axis), obj[1].dot(axis), obj[2].dot(axis), obj[3].dot(axis));

			const auto overlap_min = math::max(min1, min2);
			const auto overlap_max = math::min(max1, max2);
			return overlap_max - overlap_min;
		}

		template <typename S, typename O>
		[[nodiscard]] FORCE_INLINE constexpr bool axis_proj_overlaps(this const S& sbj, const O& obj, const vec_t axis) noexcept{
#ifdef ENABLE_SIMD
			if constexpr (std::same_as<value_type, float> && any_derived_from<O, quad>){
				if !consteval{

					alignas(32) constexpr std::int32_t PERMUTE_IDX[8] {0, 2, 4, 6, 1, 3, 5, 7};

					const __m256i idx = _mm256_load_si256(reinterpret_cast<const __m256i*>(PERMUTE_IDX));

					const __m256 sbj_data = _mm256_loadu_ps(reinterpret_cast<const float*>(&sbj));
					const __m256 obj_data = _mm256_loadu_ps(reinterpret_cast<const float*>(&obj));

					const __m256 axis_xy = _mm256_set_m128(
						_mm_set1_ps(axis.y),
						_mm_set1_ps(axis.x)
					);

					auto compute_dot_proj = [&](__m256 data) -> __m128 {
						const __m256 permuted = _mm256_permutexvar_ps(idx, data);
						const __m256 products = _mm256_mul_ps(permuted, axis_xy);
						return _mm_add_ps(_mm256_extractf128_ps(products, 0),
										 _mm256_extractf128_ps(products, 1));
					};

					const __m128 sbj_dots = compute_dot_proj(sbj_data);
					const __m128 obj_dots = compute_dot_proj(obj_data);

					const float min1 = horizontal_extreme<true>(sbj_dots);
					const float max1 = horizontal_extreme<false>(sbj_dots);
					const float min2 = horizontal_extreme<true>(obj_dots);
					const float max2 = horizontal_extreme<false>(obj_dots);

					return (max1 >= min2) && (max2 >= min1);
				}
			}
#endif

			const auto [min1, max1] = math::minmax(sbj[0].dot(axis), sbj[1].dot(axis), sbj[2].dot(axis), sbj[3].dot(axis));
			const auto [min2, max2] = math::minmax(obj[0].dot(axis), obj[1].dot(axis), obj[2].dot(axis), obj[3].dot(axis));

			return max1 >= min2 && max2 >= min1;
		}

		template <any_derived_from<quad> S, typename O, typename ...Args>
			requires (std::convertible_to<Args, vec_t> && ...) && (sizeof...(Args) > 0)
		[[nodiscard]] FORCE_INLINE bool overlap_exact(this const S& sbj, const O& obj,const Args&... args) noexcept{
			static_assert(sizeof...(Args) > 0 && sizeof...(Args) <= 8);

			return
				(sbj.axis_proj_overlaps(obj, args) && ...);
		}

		template <any_derived_from<quad> S, typename O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE constexpr bool overlap_exact(this const S& sbj, const O& obj) noexcept{
			using SbjKeys = overlay_axis_keys<S>;
			using ObjKeys = overlay_axis_keys<O>;

			return [&] <std::size_t... Idx> (std::index_sequence<Idx...>) noexcept {
				return sbj.overlap_exact(obj, math::get_axis<Idx>(sbj, obj) ...);
			}(std::make_index_sequence<SbjKeys::count + ObjKeys::count>{});
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to(this const S& sbj, const O& obj) noexcept{
			using SbjKeys = overlay_axis_keys<S>;
			using ObjKeys = overlay_axis_keys<O>;

			std::size_t index{std::numeric_limits<std::size_t>::max()};
			value_type value{std::numeric_limits<value_type>::max()};



			auto update = [&](const value_type dst, const std::size_t Idx){
				if(dst > 0 && dst < value){
					value = dst;
					index = Idx;
				}
			};

			[&] <std::size_t... Idx> (std::index_sequence<Idx...>) noexcept {
				(update(sbj.axis_proj_dst(obj, math::get_axis<Idx>(sbj, obj)), Idx), ...);
			}(std::make_index_sequence<SbjKeys::count + ObjKeys::count>{});

			if(index == std::numeric_limits<std::size_t>::max()){
				return vectors::constant2<value_type>::zero_vec2;
			}else{
				auto approach = math::get_axis(index, sbj, obj).set_length(value);

				if constexpr (SbjKeys::count + ObjKeys::count == 8){
					return approach;
				}else{
					auto avgSbj = sbj.avg();
					auto avgObj = obj.avg();

					if(approach.dot(avgObj - avgSbj) > 0){
						return -approach;
					}

					return approach;
				}
			}
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on(this const S& sbj, const O& obj, vec2 line_to_align) noexcept{
			line_to_align.normalize();

			auto proj = sbj.axis_proj_dst(obj, line_to_align);
			auto vec = line_to_align * proj;

			auto approach = obj.avg() - sbj.avg();
			if(vec.dot(approach) >= 0){
				return -vec;
			}else{
				return vec;
			}
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on_vel(this const S& sbj, const O& obj, vec_t v) noexcept{
			v.normalize();

			const auto [min_sbj, max_sbj] = math::minmax(sbj[0].dot(v), sbj[1].dot(v), sbj[2].dot(v), sbj[3].dot(v));
			const auto [min_obj, max_obj] = math::minmax(obj[0].dot(v), obj[1].dot(v), obj[2].dot(v), obj[3].dot(v));

			if(max_obj > min_sbj && min_obj < max_sbj){
				return v * (min_obj - max_sbj);
			}

			if(max_sbj > min_obj && min_sbj < max_obj){
				return v * (max_obj - min_sbj);
			}

			return {};
		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on_vel_rough_min(this const S& sbj, const O& obj, vec_t v) noexcept{
			const auto depart_vector_on_vec = sbj.depart_vector_to_on_vel(obj, v);
			const auto depart_vector = sbj.depart_vector_to(obj);

			if(depart_vector.equals({}) || depart_vector_on_vec.equals({}))return {};

			return math::lerp(depart_vector_on_vec, depart_vector,
				std::sqrt(math::curve(depart_vector_on_vec.copy().normalize().dot(depart_vector.copy().normalize()), -0.995f, 1.25f))
			);

		}

		template <any_derived_from<quad> S, any_derived_from<quad> O>
			requires requires{
				requires overlay_axis_keys<S>::count > 0;
				requires overlay_axis_keys<O>::count > 0;
			}
		[[nodiscard]] FORCE_INLINE vec_t depart_vector_to_on_vel_exact(this const S& sbj, const O& obj, vec_t v) noexcept{
			v.normalize();

			const vec_t base_depart = sbj.depart_vector_to_on_vel(obj, v) * 1.005f;

			value_type min = std::numeric_limits<value_type>::infinity();

			for(int i = 0; i < 4; ++i){
				for(int j = 0; j < 4; ++j){
					auto dst1 = math::ray_seg_intersection_dst(sbj[i] + base_depart, v, obj[j], obj[j + 1]);
					auto dst2 = math::ray_seg_intersection_dst(obj[i] - base_depart, -v, sbj[j], sbj[j + 1]);
					min = math::min(min, math::min(dst1, dst2));
				}
			}

			if(std::isinf(min)){
				return {};
			}

			assert(!std::isnan(min));
			assert(!v.is_NaN());
			assert(!base_depart.is_NaN());

			return base_depart + v * min;
		}

		[[nodiscard]] constexpr bool contains(const vec_t point) const noexcept{
			if(!get_bound().contains_loose(point)){
				return false;
			}

			bool oddNodes = false;
			constexpr float epsilon = std::numeric_limits<value_type>::epsilon(); // 浮点容差

			for (int i = 0; i < 4; ++i) {
				const auto& v0 = this->operator[](i);
				const auto& v1 = this->operator[](i + 1); // 确保 operator[] 支持循环访问

				// 跳过水平边
				if (std::abs(v1.y - v0.y) < epsilon) {
					continue;
				}

				// 检查是否跨越 point.y
				bool crosses = (v0.y <= point.y + epsilon && v1.y > point.y + epsilon) ||
							   (v1.y <= point.y + epsilon && v0.y > point.y + epsilon);

				if (crosses) {
					// 计算交点 x
					float t = (point.y - v0.y) / (v1.y - v0.y);
					float intersect_x = v0.x + t * (v1.x - v0.x);

					if (intersect_x < point.x + epsilon) {
						oddNodes = !oddNodes;
					}
				}
			}

			return oddNodes;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t v00() const noexcept{
			return v0;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t v10() const noexcept{
			return v1;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t v11() const noexcept{
			return v2;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t v01() const noexcept{
			return v3;
		}

		template <typename S, std::invocable<point2> Fn>
		void each_tile_within(this S&& self, value_type tile_extent, Fn fn){
			const rect_t& bound = self.get_bound();

			const point2 src = bound.get_src().div(tile_extent).floor().template as<int>();
			const point2 end = bound.get_end().div(tile_extent).ceil().template as<int>();

			for(int y = src.y; y != end.y; ++y){
				for(int x = src.x; x != end.x; ++x){
					const rect_t tile_bound{tags::unchecked, x * tile_extent, y * tile_extent, tile_extent, tile_extent};
					if(!self.overlap_exact(tile_bound))continue;
					if constexpr (std::is_invocable_r_v<bool, Fn, point2>){
						if(std::invoke(fn, point2{x, y})){
							return;
						}
					}else{
						std::invoke(fn, point2{x, y});
					}
				}
			}
		}
	};

	export using fquad = quad<float>;

	template <typename T>
	struct overlay_axis_keys<quad<T>>{
		static constexpr std::size_t count = 4;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const quad<T>& quad) noexcept{
			return quad.edge_normal_at(Idx);
		}
	};

	export
	template <typename T>
	constexpr quad<T> rect_ortho_to_quad(
		const rect_ortho<T>& rect, typename rect_ortho<T>::vec_t::const_pass_t center, T cos, T sin) noexcept{
		return quad<T>{
			center + (rect.vert_00() - center).rotate(cos, sin),
			center + (rect.vert_10() - center).rotate(cos, sin),
			center + (rect.vert_11() - center).rotate(cos, sin),
			center + (rect.vert_01() - center).rotate(cos, sin),
		};
	}


	export
	//TODO redesign the invariants of the class?
	template <typename T>
	struct quad_bounded : protected quad<T>{
		using base = quad<T>;

	protected:
		base::rect_t bounding_box{};

	public:
		[[nodiscard]] constexpr quad_bounded() = default;


		[[nodiscard]] constexpr explicit(false) quad_bounded(const base& base)
			: quad<T>{base}, bounding_box{base::get_bound()}{
		}

		[[nodiscard]] constexpr explicit(false) quad_bounded(base::rect_t& rect)
			: quad<T>{rect.vert_00(), rect.vert_10(), rect.vert_11(), rect.vert_01()}, bounding_box{rect}{
		}

		[[nodiscard]] constexpr explicit(false) quad_bounded(
			base::vec_t::const_pass_t v0,
			base::vec_t::const_pass_t v1,
			base::vec_t::const_pass_t v2,
			base::vec_t::const_pass_t v3

		)
			: base{v0, v1, v2, v3}, bounding_box{base::get_bound()}{
		}

		[[nodiscard]] FORCE_INLINE constexpr base::rect_t get_bound() const noexcept{
			return bounding_box;
		}

		FORCE_INLINE constexpr void move(base::vec_t::const_pass_t vec) noexcept{
			base::move(vec);

			bounding_box.src += vec;
		}

		constexpr base::vec_t expand(const T length) noexcept{
			auto rst = base::expand(length);

			bounding_box = base::get_bound();

			return rst;
		}

		/**
		 * @warning Breaks Invariant! For Fast Only
		 */
		constexpr base::vec_t expand_unchecked(const T length) noexcept{
			return base::expand(length);
		}

		constexpr explicit(false) operator base() const noexcept{
			return base{*this};
		}

		const base& view_as_quad() const noexcept{
			return *this;
		}

		template <std::integral Idx>
		FORCE_INLINE constexpr base::vec_t operator[](const Idx idx) const noexcept{
			switch (idx & vertex_mask<Idx>){
			case 0: return this->v0;
			case 1: return this->v1;
			case 2: return this->v2;
			case 3: return this->v3;
			default: std::unreachable();
			}
		}

		constexpr friend bool operator==(const quad_bounded& lhs, const quad_bounded& rhs) noexcept{
			return static_cast<const quad<T>&>(lhs) == static_cast<const quad<T>&>(rhs);
		}


		using base::avg;
		using base::axis_proj_dst;
		using base::axis_proj_overlaps;
		using base::contains;
		using base::expand;
		using base::edge_normal_at;
		using base::move;
		using base::overlap_rough;
		using base::overlap_exact;
		using base::overlap;
		using base::depart_vector_to;
		using base::depart_vector_to_on;
		using base::depart_vector_to_on_vel;
		using base::depart_vector_to_on_vel_rough_min;
		using base::depart_vector_to_on_vel_exact;
		using base::each_tile_within;
		using base::v00;
		using base::v10;
		using base::v11;
		using base::v01;

		using vec_t = base::vec_t;
		using value_type = base::value_type;
		using rect_t = base::rect_t;
	};

	export
	template <typename T>
	struct rect_box_identity{
		/**
		 * \brief Center To Bottom-Left Offset
		 */
		vector2<T> offset;

		/**
		 * \brief x for rect width, y for rect height, static
		 */
		vector2<T> size;

		constexpr rect_box_identity& operator*=(const T value) noexcept{
			offset *= value;
			size *= value;
			return *this;
		}

		constexpr rect_box_identity& operator*=(const vector2<T> value) noexcept{
			offset *= value;
			size *= value;
			return *this;
		}

		friend constexpr rect_box_identity operator*(rect_box_identity lhs, const T value) noexcept {
			return lhs *= value;
		}

		friend constexpr rect_box_identity operator*(const T value, rect_box_identity rhs) noexcept {
			return rhs *= value;
		}

		friend constexpr rect_box_identity operator*(rect_box_identity lhs, const vector2<T>& value) noexcept {
			return lhs *= value;
		}

		friend constexpr rect_box_identity operator*(const vector2<T>& value, rect_box_identity rhs) noexcept {
			return rhs *= value;
		}

		constexpr bool operator==(const rect_box_identity&) const noexcept = default;
	};

	export
	template <typename T>
	struct rect_box : quad_bounded<T>{
		using base = quad_bounded<T>;
	protected:
		/**
		 * \brief
		 * Normal Vector for v0-v1, v2-v3
		 * Edge Vector for v1-v2, v3-v0
		 */
		base::vec_t normalU{};

		/**
		 * \brief
		 * Normal Vector for v1-v2, v3-v0
		 * Edge Vector for v0-v1, v2-v3
		 */
		base::vec_t normalV{};

		constexpr void updateNormal() noexcept{
			//TODO constexpr normal in c++26
			normalU = (this->v0 - this->v3).normalize();
			normalV = (this->v1 - this->v0).normalize();
		}

	public:
		[[nodiscard]] constexpr rect_box() = default;

		[[nodiscard]] constexpr explicit(false) rect_box(const base& base)
			: quad_bounded<T>(base), normalU((this->v0 - this->v3).normalize()), normalV((this->v1 - this->v0).normalize()){
		}
		[[nodiscard]] constexpr explicit(false) rect_box(const base::rect_t& rect)
			: quad_bounded<T>(rect), normalU((this->v0 - this->v3).normalize()), normalV((this->v1 - this->v0).normalize()){
		}

		// [[nodiscard]] RectBoxBrief() = default;
		//
		[[nodiscard]] constexpr rect_box(
			const base::vec_t::const_pass_t v0,
			const base::vec_t::const_pass_t v1,
			const base::vec_t::const_pass_t v2,
			const base::vec_t::const_pass_t v3) noexcept
			: base{v0, v1, v2, v3}, normalU((this->v0 - this->v3).normalize()), normalV((this->v1 - this->v0).normalize()){
			// assert(math::abs(normalU.dot(normalV)) < 1E-2);
		}

		/**
		 * @brief normal at vtx[idx] - vtx[idx + 1]
		 */
		FORCE_INLINE constexpr base::vec_t edge_normal_at(const std::integral auto idx) const noexcept{
			switch(idx & vertex_mask<decltype(idx)>) {
				case 0 : return -normalU;
				case 1 : return normalV;
				case 2 : return normalU;
				case 3 : return -normalV;
				default: std::unreachable();
			}
		}

		constexpr void update(const math::trans2 transform, const rect_box_identity<T>& idt) noexcept{
			typename base::value_type rot = transform.rot;
			auto [cos, sin] = cos_sin(rot);

			this->v0.set(idt.offset).rotate(cos, sin);
			this->v1.set(idt.size.x, 0).rotate(cos, sin);
			this->v3.set(0, idt.size.y).rotate(cos, sin);
			this->v2 = this->v1 + this->v3;

			normalU = this->v3;
			normalV = this->v1;

			normalU.normalize();
			normalV.normalize();

			this->v0 += transform.vec;
			this->v1 += this->v0;
			this->v2 += this->v0;
			this->v3 += this->v0;

			this->bounding_box = quad<float>::get_bound();
		}
	};

	export
	using frect_box = rect_box<float>;


	template <typename T>
	struct overlay_axis_keys<rect_box<T>>{
		static constexpr std::size_t count = 2;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const rect_box<T>& quad) noexcept{
			return quad.edge_normal_at(Idx);
		}
	};

	export
	struct rect_box_posed : rect_box<float>, protected rect_box_identity<float>{
		using rect_idt_t = rect_box_identity<float>;
		using base = rect_box<float>;
		using trans_t = trans2;
		using vec_t = rect_box::vec_t;
		/**
		 * \brief Box Origin Point
		 * Should Be Mass Center if possible!
		 */
		// trans_t transform{};

		[[nodiscard]] constexpr rect_box_posed() = default;

		[[nodiscard]] constexpr explicit(false) rect_box_posed(const rect_idt_t& idt, const trans_t transform = {})
			: rect_box_identity{idt}{
			update(transform);
		}

		[[nodiscard]] constexpr explicit(false) rect_box_posed(const vec_t size, const trans_t transform = {})
			: rect_box_posed{{size / -2.f, size}, transform}{
		}

		constexpr rect_box_posed& copy_identity_from(const rect_box_posed& other) noexcept{
			this->rect_idt_t::operator=(other);
			// transform = other.transform;

			return *this;
		}

		[[nodiscard]] constexpr const rect_box_identity& get_identity() const noexcept{
			return static_cast<const rect_box_identity<float>&>(*this);
		}

		[[nodiscard]] constexpr float get_rotational_inertia(const float mass, const float scale = 1 / 12.0f, const float lengthRadiusRatio = 0.25f) const noexcept {
			return size.length2() * (scale + lengthRadiusRatio) * mass;
		}

		constexpr void update(trans_t, const rect_box_identity&) = delete;

		constexpr void update(const trans_t transform) noexcept{
			rect_box::update(transform, static_cast<const rect_box_identity&>(*this));
		}

		[[nodiscard]] constexpr trans_t deduce_transform() const noexcept{

			const vec2 rotated_offset = offset.x * normalV + offset.y * normalU;
			const vec2 translation = v0 - rotated_offset;
			const auto v = (normalV + normalU.copy().rotate_rt_counter_clockwise());

			if consteval{
				return {translation, math::atan2(v.y, v.x)};
			}else{
				return {translation, std::atan2(v.y, v.x)};
			}

		}
	};


	export
	template <>
	struct overlay_axis_keys<rect_box_posed>{
		static constexpr std::size_t count = 2;

		template <std::size_t Idx>
		FORCE_INLINE static constexpr auto get(const rect_box_posed& quad) noexcept{
			return quad.edge_normal_at(Idx);
		}
	};

	using fp_t = float;

	export
	template <typename T>
	concept quad_like = std::is_base_of_v<quad<fp_t>, T>;

	export
	rect_box<fp_t> wrap_striped_box(const vector2<fp_t> move, const rect_box<fp_t>& box) noexcept{
		if(move.equals({}, std::numeric_limits<fp_t>::epsilon() * 64)) {
			return box;
		}

		const auto ang = move.angle_rad();
		auto [cos, sin] = cos_sin(-ang);
		fp_t minX = std::numeric_limits<fp_t>::max();
		fp_t minY = std::numeric_limits<fp_t>::max();
		fp_t maxX = std::numeric_limits<fp_t>::lowest();
		fp_t maxY = std::numeric_limits<fp_t>::lowest();

		rect_box<fp_t> rst = box;

		for(std::size_t i = 0; i < 4; ++i){
			auto [x, y] = rst[i].rotate(cos, sin);
			minX = min(minX, x);
			minY = min(minY, y);
			maxX = max(maxX, x);
			maxY = max(maxY, y);
		}

		rst.move(move);

		for(std::size_t i = 0; i < 4; ++i){
			auto [x, y] = rst[i].rotate(cos, sin);
			minX = min(minX, x);
			minY = min(minY, y);
			maxX = max(maxX, x);
			maxY = max(maxY, y);
		}

		CHECKED_ASSUME(minX <= maxX);
		CHECKED_ASSUME(minY <= maxY);

		maxX += move.length();

		rst = rect_box<fp_t>{
			vector2<fp_t>{minX, minY}.rotate(cos, -sin),
			vector2<fp_t>{maxX, minY}.rotate(cos, -sin),
			vector2<fp_t>{maxX, maxY}.rotate(cos, -sin),
			vector2<fp_t>{minX, maxY}.rotate(cos, -sin)
		};

		return rst;
	}


	export
	template <quad_like T>
	[[nodiscard]] constexpr vector2<fp_t> nearest_edge_normal(const T& rectangle, const vector2<fp_t> p) noexcept {
		fp_t minDistance = std::numeric_limits<fp_t>::max();
		vector2<fp_t> closestEdgeNormal{};

		for (int i = 0; i < 4; i++) {
			auto a = rectangle[i];
			auto b = rectangle[i + 1];

			const fp_t d = math::dst2_to_segment(p, a, b);

			if (d < minDistance) {
				minDistance = d;
				closestEdgeNormal = rectangle.edge_normal_at(i);
			}
		}

		return closestEdgeNormal;
	}

	struct weighted_vector{
		fp_t weight;
		vector2<fp_t> normal;
	};

	/**
	 * @warning Return Value Is NOT Normalized!
	 */
	export
	template <quad_like T>
	[[nodiscard]] constexpr vector2<fp_t> avg_edge_normal(const T& quad, const vector2<fp_t> where) noexcept {

		std::array<weighted_vector, 4> normals{};

		for (int i = 0; i < 4; i++) {
			const vector2<fp_t> va = quad[i];
			const vector2<fp_t> vb = quad[i + 1];

			normals[i].weight = dst_to_segment(where, va, vb) * va.dst(vb);
			normals[i].normal = quad.edge_normal_at(i);
		}

		const fp_t total = (normals[0].weight + normals[1].weight + normals[2].weight + normals[3].weight);
		assert(total != 0.f);

		vector2<fp_t> closestEdgeNormal{};

		for(const auto& [weight, normal] : normals) {
			closestEdgeNormal.sub(normal * math::pow_integral<16>(weight / total));
		}

		assert(!closestEdgeNormal.is_NaN());
		if(closestEdgeNormal.is_zero()) [[unlikely]] {
			closestEdgeNormal = -std::ranges::max_element(normals, {}, &weighted_vector::weight)->normal;
		}

		return closestEdgeNormal;
	}


	export
	template <quad_like L, quad_like R>
	possible_point rect_rough_avg_intersection(const L& subject, const R& object) noexcept {
		vector2<fp_t> intersections{};
		unsigned count = 0;

		vector2<fp_t> rst{};
		for(int i = 0; i < 4; ++i) {
			for(int j = 0; j < 4; ++j) {
				if(math::intersect_segments(subject[i], subject[(i + 1)], object[j], object[(j + 1)], rst)) {
					count++;
					intersections += rst;
				}
			}
		}

		if(count > 0) {
			return {intersections.div(static_cast<fp_t>(count)), true};
		}

		return {};
	}

}

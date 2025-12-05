module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.math.trans2;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math;
export import mo_yanxi.math.angle;

import mo_yanxi.concepts;
import std;

export namespace mo_yanxi::math{
	template <typename AngTy>
	struct transform2 {
		using angle_t = AngTy;
		using vec_t = vec2;

		vec_t vec;
		angle_t rot;

		FORCE_INLINE constexpr void set_zero(){
			vec.set_zero();
			rot = 0.0f;
		}

		// template <float NaN = std::numeric_limits<angle_t>::signaling_NaN()>
		// FORCE_INLINE constexpr void set_nan() noexcept requires std::is_floating_point_v<angle_t>{
		// 	vec.set(NaN, NaN);
		// 	// rot = NaN;
		// }

		[[nodiscard]] FORCE_INLINE constexpr bool is_NaN() const noexcept {
			return vec.is_NaN() || std::isnan(rot);
		}

		template <typename Ang>
		FORCE_INLINE constexpr transform2& apply_inv(const transform2<Ang>& debaseTarget) noexcept{
			vec.sub(debaseTarget.vec).rotate_rad(-static_cast<float>(debaseTarget.rot));
			rot -= debaseTarget.rot;

			return *this;
		}

		template <typename Ang>
		FORCE_INLINE constexpr transform2& apply(const transform2<Ang>& rebaseTarget) noexcept{
			vec.rotate_rad(static_cast<float>(rebaseTarget.rot)).add(rebaseTarget.vec);
			rot += rebaseTarget.rot;

			return *this;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t apply_inv_to(vec_t debaseTarget) const noexcept{
			debaseTarget.sub(vec).rotate_rad(-static_cast<float>(rot));
			return debaseTarget;
		}

		template <typename Ang>
		[[nodiscard]] FORCE_INLINE constexpr transform2<Ang> apply_inv_to(transform2<Ang> debaseTarget) const noexcept{
			debaseTarget.apply_inv(*this);
			return debaseTarget;
		}

		[[nodiscard]] FORCE_INLINE constexpr vec_t apply_to(vec_t target) const noexcept{
			target.rotate_rad(static_cast<float>(rot)).add(vec);
			return target;
		}

		FORCE_INLINE constexpr transform2& operator|=(const transform2& parentRef) noexcept{
			return transform2::apply(parentRef);
		}

		FORCE_INLINE constexpr transform2& operator+=(const transform2 other) noexcept{
			vec += other.vec;
			rot += other.rot;

			return *this;
		}

		FORCE_INLINE constexpr transform2& operator+=(const vec_t other) noexcept{
			vec += other;

			return *this;
		}

		FORCE_INLINE constexpr transform2& operator-=(const transform2 other) noexcept{
			vec -= other.vec;
			rot -= other.rot;

			return *this;
		}

		FORCE_INLINE constexpr transform2& operator*=(const float scl) noexcept{
			vec *= scl;
			rot *= scl;

			return *this;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator*(transform2 self, const float scl) noexcept{
			return self *= scl;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator+(transform2 self, const transform2 other) noexcept{
			return self += other;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator-(transform2 self, const transform2 other) noexcept{
			return self -= other;
		}

		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator-(transform2 self) noexcept{
			self.vec.reverse();
			self.rot = -self.rot;
			return self;
		}

		template <typename T>
			requires (std::convertible_to<angle_t, T>)
		FORCE_INLINE constexpr explicit(false) operator transform2<T>() const noexcept{
			return transform2<T>{vec, T(rot)};
		}

		/**
		 * @brief Local To Parent
		 * @param self To Trans
		 * @param parentRef Parent Frame Reference Trans
		 * @return Transformed Translation
		 */
		template <typename R>
		[[nodiscard]] FORCE_INLINE constexpr friend transform2 operator|(transform2 self, const transform2<R>& parentRef) noexcept{
			return self |= parentRef;
		}

		FORCE_INLINE constexpr friend vec_t& operator|=(vec_t& vec, const transform2 transform) noexcept{
			return vec.rotate_rad(static_cast<float>(transform.rot)).add(transform.vec);
		}


		constexpr friend bool operator==(const transform2& lhs, const transform2& rhs) noexcept = default;

		[[nodiscard]] constexpr cos_sin_result<float> rot_cos_sin() const noexcept{
			return math::cos_sin(rot);
		}

		[[nodiscard]] constexpr bool within(const transform2& rhs, float dst_margin, float ang_margin) const noexcept{
			return vec.within(rhs.vec, dst_margin) && math::angles::angle_distance_abs(rot, rhs.rot) < ang_margin;
		}

		friend constexpr transform2 lerp(const transform2& lhs, const transform2& rhs, float p){
			return {
				lerp(lhs.vec, rhs.vec, p),
				static_cast<angle_t>(lerp(uniformed_angle{lhs.rot}, uniformed_angle{rhs.rot}, p))
				};
		}
	};


	template <typename T>
	[[nodiscard]] FORCE_INLINE constexpr vec2 operator|(vec2 vec, const transform2<T>& transform) noexcept{
		return vec |= transform;
	}

	template <typename AngTy>
	struct transform2z : transform2<AngTy>{
		float z_offset;

		using trans_t = transform2<AngTy>;
		using transform2<AngTy>::operator|=;

		[[nodiscard]] FORCE_INLINE constexpr trans_t get_trans() const noexcept{
			return static_cast<trans_t>(*this);
		}

		template <typename T>
		FORCE_INLINE constexpr transform2z& operator|=(const transform2z<T>& parentRef) noexcept{
			trans_t::operator|=(static_cast<trans_t>(parentRef)); // NOLINT(*-slicing)
			z_offset += parentRef.z_offset;

			return *this;
		}

		template <typename T>
		[[nodiscard]] FORCE_INLINE  constexpr friend transform2z operator|(transform2z self, const transform2z<T> parentRef) noexcept{
			return self |= parentRef;
		}

		template <typename T>
		FORCE_INLINE constexpr transform2z& operator+=(const transform2z<T>& parentRef) noexcept{
			trans_t::operator+=(static_cast<trans_t>(parentRef)); // NOLINT(*-slicing)
			z_offset += parentRef.z_offset;

			return *this;
		}

		template <typename T>
		FORCE_INLINE constexpr transform2z friend operator+( transform2z lhs, const transform2z<T>& rhs) noexcept{
			return lhs += rhs;
		}

		[[nodiscard]] FORCE_INLINE constexpr transform2z operator*(const float scl) const noexcept{
			transform2z state = *this;
			state.trans_t::operator*=(scl);
			state.z_offset *= scl;

			return state;
		}
	};


	struct pos_transform2z{
		math::vec2 vec;
		float z_offset;

		template <typename AngTy>
		FORCE_INLINE constexpr pos_transform2z& operator|=(const transform2z<AngTy>& parentRef) noexcept{
			vec |= parentRef.get_trans();
			z_offset += parentRef.z_offset;

			return *this;
		}

		template <typename AngTy>
		[[nodiscard]] FORCE_INLINE constexpr friend pos_transform2z operator|(pos_transform2z self, const transform2z<AngTy>& parentRef) noexcept{
			return self |= parentRef;
		}

		template <typename AngTy>
		FORCE_INLINE constexpr pos_transform2z& operator|=(const transform2<AngTy>& rhs) noexcept{
			vec |= rhs;

			return *this;
		}

		template <typename AngTy>
		[[nodiscard]] FORCE_INLINE constexpr friend pos_transform2z operator|(pos_transform2z self, const transform2<AngTy>& parentRef) noexcept{
			return self |= parentRef;
		}

		FORCE_INLINE constexpr pos_transform2z& operator+=(const pos_transform2z lhs) noexcept{
			vec += lhs.vec;
			z_offset += lhs.z_offset;

			return *this;
		}

		FORCE_INLINE constexpr pos_transform2z friend operator+(pos_transform2z lhs, const pos_transform2z& rhs) noexcept{
			return lhs += rhs;
		}


	};



	using trans2 = transform2<float>;
	using trans2z = transform2z<float>;
	using pos_trans2z = pos_transform2z;
	using uniform_trans2 = transform2<angle>;
}

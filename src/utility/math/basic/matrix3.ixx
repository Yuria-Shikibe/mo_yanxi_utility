module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.math.matrix3;

import std;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.vector3;
export import mo_yanxi.math.trans2;
export import mo_yanxi.math;

namespace mo_yanxi::math{
export
struct matrix3{
	using floating_point_t = float;
	using value_type = floating_point_t;
	using vec2_t = vector2<floating_point_t>;
	using vec3_t = vector3<floating_point_t>;
	using trans2_t = transform2<floating_point_t>;

	vec3_t c1, c2, c3;

private:
	//TODO OPTM SIMD
	FORCE_INLINE static constexpr void mul(matrix3& rhs, const matrix3& to_apply){
		const vec3_t c1 = to_apply.c1 * rhs.c1.x + to_apply.c2 * rhs.c1.y + to_apply.c3 * rhs.c1.z;
		const vec3_t c2 = to_apply.c1 * rhs.c2.x + to_apply.c2 * rhs.c2.y + to_apply.c3 * rhs.c2.z;
		const vec3_t c3 = to_apply.c1 * rhs.c3.x + to_apply.c2 * rhs.c3.y + to_apply.c3 * rhs.c3.z;

		rhs.c1 = c1;
		rhs.c2 = c2;
		rhs.c3 = c3;
	}

public:
	friend constexpr bool operator==(const matrix3& lhs, const matrix3& rhs) noexcept = default;

	FORCE_INLINE friend constexpr vec2_t operator*(const matrix3& mat, const vec2_t vec) noexcept{
		return vec.x * static_cast<vec2_t>(mat.c1) + vec.y * static_cast<vec2_t>(mat.c2) + static_cast<vec2_t>(mat.c3);
	}

	FORCE_INLINE friend constexpr matrix3& operator*(const vec2_t vec, matrix3& mat) noexcept{
		return mat.scale(vec);
	}

	FORCE_INLINE friend constexpr vec2_t& operator*=(vec2_t& vec, const matrix3& mat) noexcept{
		return vec = vec.x * static_cast<vec2_t>(mat.c1) + vec.y * static_cast<vec2_t>(mat.c2) + static_cast<vec2_t>(mat
			.c3);
	}

	FORCE_INLINE friend constexpr matrix3& operator*=(matrix3& lhs, const matrix3& rhs) noexcept{
		return lhs.apply(rhs);
	}

	FORCE_INLINE friend constexpr matrix3& operator+=(matrix3& lhs, const matrix3& rhs) noexcept{
		lhs.c1 += rhs.c1;
		lhs.c2 += rhs.c2;
		lhs.c3 += rhs.c3;
		return lhs;
	}

	FORCE_INLINE friend constexpr matrix3& operator-=(matrix3& lhs, const matrix3& rhs) noexcept{
		lhs.c1 -= rhs.c1;
		lhs.c2 -= rhs.c2;
		lhs.c3 -= rhs.c3;
		return lhs;
	}

	FORCE_INLINE friend constexpr matrix3 operator+(const matrix3& lhs, const matrix3& rhs) noexcept{
		auto dcpy = lhs;
		return dcpy += rhs;
	}

	FORCE_INLINE friend constexpr matrix3 operator-(const matrix3& lhs, const matrix3& rhs) noexcept{
		auto dcpy = lhs;
		return dcpy -= rhs;
	}

	FORCE_INLINE friend constexpr matrix3 operator*(const matrix3& lhs, const matrix3& rhs) noexcept{
		auto dcpy = rhs;
		return dcpy.apply(lhs);
	}

	// FORCE_INLINE constexpr matrix3& operator*=(const matrix3& lhs) noexcept {
	// 	return apply(lhs);
	// }

	FORCE_INLINE constexpr matrix3& operator~() noexcept{
		return inv();
	}

	FORCE_INLINE constexpr floating_point_t operator*() const noexcept{
		return det();
	}

	friend std::ostream& operator<<(std::ostream& os, const matrix3& obj){
		return os
			<< "[" << obj.c1.x << "|" << obj.c2.x << "|" << obj.c2.x << "]\n" //
			<< "[" << obj.c1.y << "|" << obj.c2.y << "|" << obj.c2.y << "]\n" //
			<< "[" << obj.c1.z << "|" << obj.c2.z << "|" << obj.c3.z << "]";
	}

	FORCE_INLINE constexpr matrix3& from_transform(const trans2_t translation) noexcept{
		from_rotation_rad(translation.rot);
		c3.x = translation.vec.x;
		c3.y = translation.vec.y;
		c3.z = 1;
		return *this;
	}

	[[nodiscard]] FORCE_INLINE trans2_t to_transform() const noexcept{
		return trans2_t{get_translation(), get_rotation_rad()};
	}

	FORCE_INLINE constexpr matrix3& transform(const trans2_t translation) noexcept{
		return rotate_rad(translation.rot).move(translation.vec);
	}

	FORCE_INLINE constexpr auto operator[](const std::size_t index_column_major) const noexcept{
		assert(index_column_major < 9);
		switch(index_column_major){
		case 0 : return c1.x;
		case 1 : return c1.y;
		case 2 : return c1.z;
		case 3 : return c2.x;
		case 4 : return c2.y;
		case 5 : return c2.z;
		case 6 : return c3.x;
		case 7 : return c3.y;
		case 8 : return c3.z;
		default : std::unreachable();
		}
	}

	constexpr matrix3& set_orthogonal(const vec2_t bot_lft, const vec2_t size) noexcept{
		const auto top_rit = bot_lft + size;

		const auto orth = ~size * static_cast<vec2_t::floating_point_t>(2.);

		return from_scaling(orth).set_translation(-(top_rit + bot_lft) / size);
	}

	constexpr matrix3& set_orthogonal_flip_y(vec2_t bot_lft, vec2_t size) noexcept{
		bot_lft.add_y(size.y);
		return set_orthogonal(bot_lft, size.flip_y());
	}

	constexpr matrix3& set_orthogonal(const vec2_t size) noexcept{
		return set_orthogonal({}, size);
	}

	FORCE_INLINE constexpr matrix3& idt() noexcept{
		return this->operator=(matrix3{
				1, 0, 0,
				0, 1, 0,
				0, 0, 1
			});
	}

	FORCE_INLINE constexpr matrix3& apply(const matrix3& transform) noexcept{
		mul(*this, transform);

		return *this;
	}

	FORCE_INLINE constexpr matrix3& apply_ortho(const matrix3& transform) noexcept{
		c1.x *= transform.c1.x;
		c2.y *= transform.c2.y;
		c3.x += transform.c3.x;
		c3.y += transform.c3.y;

		return *this;
	}

	FORCE_INLINE constexpr matrix3& from_rotation_deg(const floating_point_t degrees) noexcept{
		return from_rotation_rad(math::deg_to_rad_v<floating_point_t> * degrees);
	}

	FORCE_INLINE constexpr matrix3& from_rotation_rad(const floating_point_t radians) noexcept{
		return set_rotation_rad(radians).set_translation({});
	}

	FORCE_INLINE constexpr matrix3& set_rotation_deg(const floating_point_t degrees) noexcept{
		return set_rotation_rad(math::deg_to_rad_v<floating_point_t> * degrees);
	}

	FORCE_INLINE constexpr matrix3& set_rotation_rad(const floating_point_t radians) noexcept{
		auto [cos, sin] = math::cos_sin(radians);

		c1 = {cos, sin, 0};
		c2 = {-sin, cos, 0};

		return *this;
	}

	FORCE_INLINE constexpr matrix3& from_axis_rotation(const Vec3 axis, const float degrees) noexcept{
		return from_axis_rotation(axis, math::cos_deg(degrees), math::sin_deg(degrees));
	}

	FORCE_INLINE constexpr matrix3& from_axis_rotation(const Vec3 axis, const float cos, const float sin) noexcept{
		const float oc = 1.0f - cos;
		c1.x = oc * axis.x * axis.x + cos;
		c1.y = oc * axis.x * axis.y - axis.z * sin;
		c1.z = oc * axis.z * axis.x + axis.y * sin;
		c2.x = oc * axis.x * axis.y + axis.z * sin;
		c2.y = oc * axis.y * axis.y + cos;
		c2.z = oc * axis.y * axis.z - axis.x * sin;
		c3.x = oc * axis.z * axis.x - axis.y * sin;
		c3.y = oc * axis.y * axis.z + axis.x * sin;
		c3.z = oc * axis.z * axis.z + cos;
		return *this;
	}

	FORCE_INLINE constexpr matrix3& translate(const float x, const float y) noexcept{
		return apply({
				{1, 0, 0},
				{0, 1, 0},
				{x, y, 1},
			});
	}

	FORCE_INLINE constexpr matrix3& translate(const vec2_t translation) noexcept{
		return translate(translation.x, translation.y);
	}

	FORCE_INLINE constexpr matrix3& from_translation(const vec2_t translation) noexcept{
		return from_translation(translation.x, translation.y);
	}

	FORCE_INLINE constexpr matrix3& from_translation(const float x, const float y) noexcept{
		c1 = {1, 0, 0};
		c2 = {0, 1, 0};
		c3 = {x, y, 1};

		return *this;
	}

	FORCE_INLINE constexpr matrix3& set_translation(const vec2_t translation) noexcept{
		return set_translation(translation.x, translation.y);
	}

	FORCE_INLINE constexpr matrix3& set_translation(const float x, const float y) noexcept{
		c3 = {x, y, 1};

		return *this;
	}

	FORCE_INLINE constexpr matrix3& from_scaling(const value_type scaleX, const value_type scaleY) noexcept{
		c1 = {scaleX, 0, 0};
		c2 = {0, scaleY, 0};
		c3 = {0, 0, 1};
		return *this;
	}

	FORCE_INLINE constexpr matrix3& from_scaling(const value_type scale) noexcept{
		return from_scaling(scale, scale);
	}

	FORCE_INLINE constexpr matrix3& from_scaling(const vec2_t scale) noexcept{
		return from_scaling(scale.x, scale.y);
	}

	FORCE_INLINE constexpr matrix3& set_scaling(const value_type scaleX, const value_type scaleY) noexcept{
		const auto det2 = c1.x * c2.y - c2.y * c1.x;

		c1 *= scaleX / det2;
		c2 *= scaleY / det2;
		return *this;
	}

	FORCE_INLINE constexpr matrix3& set_scaling(const value_type scale) noexcept{
		return set_scaling(scale, scale);
	}

	FORCE_INLINE constexpr matrix3& set_scaling(const vec2_t scale) noexcept{
		return set_scaling(scale.x, scale.y);
	}

	[[nodiscard]] std::string toString() const noexcept{
		std::stringstream ss;

		ss << *this;

		return std::move(ss).str();
	}

	[[nodiscard]] FORCE_INLINE constexpr vec3_t::value_type det() const noexcept{
		return c1.x * c2.y * c3.z + c1.y * c2.z * c3.x + c1.z * c2.x * c3.y - c1.x * c3.y * c2.z - c1.y * c3.z * c2.x -
			c1.z * c3.x * c2.y;
	}

	/**
	* @brief Only works when this mat is rot * square scale + trans
	* */
	FORCE_INLINE constexpr matrix3& inv_ortho_transform() noexcept{
		std::swap(c1.y, c2.x);
		c3.x *= -1;
		c3.y *= -1;

		return *this;
	}

	FORCE_INLINE constexpr matrix3& inv_scaled_transform() noexcept{
		c1.x = 1 / c1.x;
		c1.y = 1 / c1.y;
		c2.x = 1 / c1.x;
		c2.y = 1 / c1.y;

		c3.x *= -1;
		c3.y *= -1;

		return *this;
	}


	/**
	* @brief Only works when this mat is rot * square scale + trans
	* */
	[[nodiscard]] FORCE_INLINE constexpr matrix3 get_inv_ortho_transform() const noexcept{
		matrix3 cpy{*this};

		return cpy.inv_ortho_transform();
	}

	FORCE_INLINE constexpr matrix3& inv() noexcept{
		const auto detV = det();

		if(detV == 0.){
			idt();
			return *this;
		}

		const floating_point_t inv_det = 1.0f / static_cast<floating_point_t>(detV);


		vec3_t tc1{
				c2.y * c3.z - c2.z * c3.y,
				c1.z * c3.y - c1.y * c3.z,
				c1.y * c2.z - c1.z * c2.y
			};

		vec3_t tc2{
				c2.z * c3.x - c3.z * c2.x,
				c1.x * c3.z - c1.z * c3.x,
				c1.z * c2.x - c1.x * c2.z
			};

		vec3_t tc3{
				c2.x * c3.y - c2.y * c3.x,
				c1.y * c3.x - c1.x * c3.y,
				c1.x * c2.y - c1.y * c2.x
			};

		c1 = tc1 * inv_det;
		c2 = tc2 * inv_det;
		c3 = tc3 * inv_det;

		return *this;
	}

	constexpr matrix3& set(const matrix3& mat) noexcept{
		return this->operator=(mat);
	}

	FORCE_INLINE constexpr matrix3& move(const vec2_t translation) noexcept{
		c3 += static_cast<vec3_t>(translation);
		c3.z = 1;
		return *this;
	}

	FORCE_INLINE constexpr matrix3& rotate_deg(const floating_point_t degrees) noexcept{
		return rotate_rad(math::deg_to_rad_v<floating_point_t> * degrees);
	}

	FORCE_INLINE constexpr matrix3& rotate_rad(const floating_point_t radians) noexcept{
		if(radians == 0) [[unlikely]] return *this;
		mul(*this, matrix3{}.from_rotation_rad(radians));
		return *this;
	}

	FORCE_INLINE constexpr matrix3& scale(const floating_point_t scaleX, const floating_point_t scaleY) noexcept{
		mul(*this, {
				{scaleX, 0, 0},
				{0, scaleY, 0},
				{0, 0, 1},
			});
		return *this;
	}

	FORCE_INLINE constexpr matrix3& scale(const vec2_t scl) noexcept{
		return scale(scl.x, scl.y);
	}

	[[nodiscard]] FORCE_INLINE constexpr vec2_t get_translation() const noexcept{
		return static_cast<vec2_t>(c3);
	}

	[[nodiscard]] FORCE_INLINE vec2_t get_scale() const noexcept{
		return {static_cast<vec2_t>(c1).length(), static_cast<vec2_t>(c2).length()};
	}

	[[nodiscard]] FORCE_INLINE vec2_t get_ortho_scale() const noexcept{
		return {c1.x, c2.y};
	}

	[[nodiscard]] FORCE_INLINE floating_point_t get_rotation_deg() const noexcept{
		return math::rad_to_deg_v<floating_point_t> * get_rotation_rad();
	}

	[[nodiscard]] FORCE_INLINE constexpr floating_point_t get_rotation_rad() const noexcept{
		return math::atan2(c1.y, c1.x);
	}

	//
	// constexpr matrix3& scl(const float scale) noexcept{
	// 	val[M00] *= scale;
	// 	val[M11] *= scale;
	// 	return *this;
	// }
	//
	// constexpr matrix3& scl(const vec2_t& scale) noexcept{
	// 	val[M00] *= scale.x;
	// 	val[M11] *= scale.y;
	// 	return *this;
	// }
	//
	// FORCE_INLINE constexpr matrix3& scl(const Vec3& scale) noexcept{
	// 	val[M00] *= scale.x;
	// 	val[M11] *= scale.y;
	// 	return *this;
	// }

	FORCE_INLINE constexpr matrix3& transpose() noexcept{
		// Where MXY you do not have to change MXX

		std::swap(c2.x, c1.y);
		std::swap(c3.x, c1.z);
		std::swap(c3.y, c2.z);

		return *this;
	}

	constexpr matrix3& set_rect_transform(const vec2_t src, const vec2_t src_extent, const vec2_t dst,
		const vec2_t dst_extent) noexcept{
		const auto scl = dst_extent / src_extent;
		const auto mov = dst - src * scl;

		c1 = {scl.x, 0, 0};
		c2 = {0, scl.y, 0};
		c3 = {mov.x, mov.y, 1};
		return *this;
	}
};

export using mat3 = matrix3;
export constexpr matrix3 mat3_idt{1, 0, 0, 0, 1, 0, 0, 0, 1};
}

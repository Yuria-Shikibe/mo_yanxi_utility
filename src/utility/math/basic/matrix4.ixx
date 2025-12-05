module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.math.matrix4;

import std;

export import mo_yanxi.math.vector4;
export import mo_yanxi.math;

namespace mo_yanxi::math{
	export
	struct matrix4{
		vec4 c0, c1, c2, c3;

		FORCE_INLINE constexpr matrix4 operator-() const noexcept{
			return {-c0, -c1, -c2, -c3};
		}

		// 复合赋值运算符
		FORCE_INLINE constexpr matrix4& operator+=(const matrix4& other) noexcept{
			c0 += other.c0;
			c1 += other.c1;
			c2 += other.c2;
			c3 += other.c3;
			return *this;
		}

		FORCE_INLINE constexpr matrix4& operator-=(const matrix4& other) noexcept{
			c0 -= other.c0;
			c1 -= other.c1;
			c2 -= other.c2;
			c3 -= other.c3;
			return *this;
		}

		FORCE_INLINE constexpr matrix4& operator*=(const matrix4& other) noexcept{
			c0 = other * c0;
			c1 = other * c1;
			c2 = other * c2;
			c3 = other * c3;
			return *this;
		}

		FORCE_INLINE constexpr matrix4& operator*=(const float other) noexcept{
			c0 = c0 * other;
			c1 = c1 * other;
			c2 = c2 * other;
			c3 = c3 * other;
			return *this;
		}

		FORCE_INLINE friend constexpr vec4 operator*=(const vec4& v, const matrix4& self) noexcept{
			return {
					self.c0.r * v.r + self.c1.r * v.g + self.c2.r * v.b + self.c3.r * v.a,
					self.c0.g * v.r + self.c1.g * v.g + self.c2.g * v.b + self.c3.g * v.a,
					self.c0.b * v.r + self.c1.b * v.g + self.c2.b * v.b + self.c3.b * v.a,
					self.c0.a * v.r + self.c1.a * v.g + self.c2.a * v.b + self.c3.a * v.a
				};
		}

		FORCE_INLINE constexpr vec4 operator*(const vec4 v) const noexcept{
			return v *= *this;
		}

		FORCE_INLINE friend constexpr matrix4 operator+(matrix4 lhs, const matrix4& rhs) noexcept{
			lhs += rhs;
			return lhs;
		}

		FORCE_INLINE friend constexpr matrix4 operator-(matrix4 lhs, const matrix4& rhs) noexcept{
			lhs -= rhs;
			return lhs;
		}

		FORCE_INLINE friend constexpr matrix4 operator*(matrix4 lhs, const matrix4& rhs) noexcept{
			return {
				rhs.c0 *= lhs,
				rhs.c1 *= lhs,
				rhs.c2 *= lhs,
				rhs.c3 *= lhs,
			};
		}

		FORCE_INLINE friend constexpr matrix4 operator*(matrix4 lhs, const float rhs) noexcept{
			return lhs *= rhs;
		}

		FORCE_INLINE friend constexpr matrix4 operator*(const float lhs, matrix4 rhs) noexcept{
			return rhs *= lhs;
		}
	};

};

module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.math.vector4;

import std;
import mo_yanxi.math;
import mo_yanxi.concepts;
// import Geom.Vector3D;


namespace mo_yanxi::math{
		export
		template <typename T>
		struct vector4{
			T r;
			T g;
			T b;
			T a;

			using value_type = T;
			using floating_point_t = std::conditional_t<std::floating_point<T>, T,
														std::conditional_t<sizeof(T) <= 4, float, double>
			>;
			using const_pass_t = mo_yanxi::conditional_pass_type<const vector4, sizeof(T) * 4>;

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator+(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator-(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator-(const D& lhs) noexcept{
				return D{-lhs.r, -lhs.g, -lhs.b, -lhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator*(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator/(const D& lhs, const D& rhs) noexcept{
				return D{lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b, lhs.a / rhs.a};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator%(const D& lhs, const D& rhs) noexcept{
				return {math::mod(lhs.r, rhs.r), math::mod(lhs.g, rhs.g), math::mod(lhs.b, rhs.b), math::mod(lhs.a, rhs.a)};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator*(const D& lhs, const T val) noexcept{
				return {lhs.r * val, lhs.g * val, lhs.b * val, lhs.a * val};
			}


			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator*(const T val, const D& rhs) noexcept{
				return {rhs.r * val, rhs.g * val, rhs.b * val, rhs.a * val};
			}


			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator/(const D& lhs, const T val) noexcept{
				return {lhs.r / val, lhs.g / val, lhs.b / val, lhs.a / val};
			}

			template <std::derived_from<vector4> D>
			[[nodiscard]] FORCE_INLINE friend constexpr D operator%(const D& lhs, const T val) noexcept{
				return {math::mod(lhs.r, val), math::mod(lhs.g, val), math::mod(lhs.b, val), math::mod(lhs.a, val)};
			}


			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator+=(D& lhs, const D& rhs) noexcept{
				lhs.r += rhs.r;
				lhs.g += rhs.g;
				lhs.b += rhs.b;
				lhs.a += rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator-=(D& lhs, const D& rhs) noexcept{
				lhs.r -= rhs.r;
				lhs.g -= rhs.g;
				lhs.b -= rhs.b;
				lhs.a -= rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator*=(D& lhs, const D& rhs) noexcept{
				lhs.r *= rhs.r;
				lhs.g *= rhs.g;
				lhs.b *= rhs.b;
				lhs.a *= rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator/=(D& lhs, const D& rhs) noexcept{
				lhs.r /= rhs.r;
				lhs.g /= rhs.g;
				lhs.b /= rhs.b;
				lhs.a /= rhs.a;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator%=(D& lhs, const D& rhs) noexcept{
				lhs.r = math::mod(lhs.r, rhs.r);
				lhs.g = math::mod(lhs.g, rhs.g);
				lhs.b = math::mod(lhs.b, rhs.b);
				lhs.a = math::mod(lhs.a, rhs.a);
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator*=(D& lhs, const T v) noexcept{
				lhs.r *= v;
				lhs.g *= v;
				lhs.b *= v;
				lhs.a *= v;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator/=(D& lhs, const T v) noexcept{
				lhs.r /= v;
				lhs.g /= v;
				lhs.b /= v;
				lhs.a /= v;
				return lhs;
			}

			template <std::derived_from<vector4> D>
			FORCE_INLINE friend constexpr D& operator%=(D& lhs, const T v) noexcept{
				lhs.r = math::mod(lhs.r, v);
				lhs.g = math::mod(lhs.g, v);
				lhs.b = math::mod(lhs.b, v);
				lhs.a = math::mod(lhs.a, v);
				return lhs;
			}

			FORCE_INLINE constexpr vector4& set(const T ox, const T oy, const T oz, const T ow) noexcept{
				this->r = ox;
				this->g = oy;
				this->b = oz;
				this->a = ow;

				return *this;
			}

			template <typename S>
			FORCE_INLINE constexpr S& set(this S& self, const T val) noexcept{
				return self.set(val, val, val, val);
			}

			template <typename S>
			FORCE_INLINE constexpr S& set(this S& self, const_pass_t tgt) noexcept{
				return (self = tgt);
			}

			// template <typename S>
			FORCE_INLINE constexpr vector4& lerp(this vector4& self, const vector4& tgt, const floating_point_t alpha) noexcept{
				return (self = mo_yanxi::math::lerp(self, tgt, alpha));
			}


			template <typename S>
			FORCE_INLINE constexpr S& clamp(this S& self) noexcept{
				self.r = math::clamp(self.r);
				self.g = math::clamp(self.g);
				self.b = math::clamp(self.b);
				self.a = math::clamp(self.a);

				return self;
			}

			constexpr friend bool operator==(const vector4& lhs, const vector4& rhs) noexcept = default;
		};

		export using vec4 = vector4<float>;
	}


module;

#include "../../../../include/mo_yanxi/adapted_attributes.hpp"
#define ATTR FORCE_INLINE PURE_FN

export module mo_yanxi.math.vector2;

import mo_yanxi.math;
import mo_yanxi.concepts;
import std;

namespace mo_yanxi::math{
	export
	template <typename T>
	struct vec2_products{
		T dot;
		T cross;

		void to_trigonometric(T length) noexcept{
			dot /= length;
			cross /= length;
		}
	};

	export
	template <typename T>
	struct vector2
	{
		T x;
		T y;

		using value_type = T;
		using floating_point_t = std::conditional_t<std::floating_point<T>, T,
			std::conditional_t<sizeof(T) <= 4, float, double>
		>;

		using const_pass_t = mo_yanxi::conditional_pass_type<const vector2, sizeof(T) * 2>;

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 operator+(const_pass_t tgt) const noexcept{
			return {x + tgt.x, y + tgt.y};
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 operator-(const_pass_t tgt) const noexcept {
			if constexpr(std::same_as<bool, T>){
				return {x && !tgt.x, y && !tgt.y};
			}else{
				return {x - tgt.x, y - tgt.y};
			}
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 operator*(const_pass_t tgt) const noexcept {
			return {x * tgt.x, y * tgt.y};
		}

		FORCE_INLINE friend constexpr vector2 operator*(const_pass_t v, const T val) noexcept {
			return {v.x * val, v.y * val};
		}

		FORCE_INLINE friend constexpr vector2 operator*(const T val, const_pass_t v) noexcept {
			return {v.x * val, v.y * val};
		}

		FORCE_INLINE constexpr vector2 operator!() const noexcept requires (std::same_as<T, bool>){
			return {!x, !y};
		}

		/*template <ext::number N>
		[[nodiscard]] constexpr auto operator*(const N val) const noexcept {
			if(std::is_unsigned_v<T> && !std::is_unsigned_v<N>){
				using S = std::make_signed_t<T>;
				using R = std::common_type_t<S, N>;

				return Vector2D<R>{static_cast<R>(x) * static_cast<R>(val), static_cast<R>(y) * static_cast<R>(val)};
			}
			return Vector2D{x * val, y * val};
		}*/

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr auto operator-() const noexcept{
			if constexpr (std::unsigned_integral<T>){
				using S = std::make_signed_t<T>;
				return vector2<std::make_signed_t<T>>{-static_cast<S>(x), -static_cast<S>(y)};
			}else{
				return vector2<T>{- x, - y};
			}

		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 operator/(const T val) const noexcept {
			return {x / val, y / val};
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 operator/(const_pass_t tgt) const noexcept {
			return {x / tgt.x, y / tgt.y};
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 operator%(const_pass_t tgt) const noexcept {
			return {math::mod<T>(x, tgt.x), math::mod<T>(y, tgt.y)};
		}

		FORCE_INLINE constexpr vector2& operator+=(const_pass_t tgt) noexcept {
			return this->add(tgt);
		}

		PURE_FN FORCE_INLINE constexpr vector2 operator~() const noexcept{
			if constexpr (std::floating_point<T>){
				return {static_cast<T>(1) / x, static_cast<T>(1) / y};
			}else if constexpr (std::unsigned_integral<T>){
				return {~x, ~y};
			}else{
				static_assert(false, "unsupported");
			}
		}

		FORCE_INLINE constexpr vector2& inverse() noexcept{
			return this->set(this->operator~());
		}

		template <std::floating_point Ty = float>
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2<Ty>& reciprocal() const noexcept{
			return ~as<Ty>();
		}

		FORCE_INLINE constexpr vector2& operator+=(const T tgt) noexcept {
			return this->add(tgt);
		}

		FORCE_INLINE constexpr vector2& operator-=(const_pass_t tgt) noexcept {
			return this->sub(tgt);
		}

		FORCE_INLINE constexpr vector2& operator*=(const_pass_t tgt) noexcept {
			return this->mul(tgt);
		}

		FORCE_INLINE constexpr vector2& operator*=(const T val) noexcept {
			return this->mul(val);
		}

		FORCE_INLINE constexpr vector2& operator/=(const_pass_t tgt) noexcept {
			return this->div(tgt);
		}

		FORCE_INLINE constexpr vector2& operator/=(const T tgt) noexcept {
			return this->div(tgt, tgt);
		}

		FORCE_INLINE constexpr vector2& operator%=(const_pass_t tgt) noexcept {
			return this->mod(tgt.x, tgt.y);
		}

		FORCE_INLINE constexpr vector2& operator%=(const T tgt) noexcept {
			return this->mod(tgt, tgt);
		}

		FORCE_INLINE constexpr vector2& inf_to0() noexcept{
			if(std::isinf(x))x = 0;
			if(std::isinf(y))y = 0;
			return *this;
		}

		FORCE_INLINE constexpr vector2& nan_to(const_pass_t value) noexcept{
			if(std::isnan(x))x = value.x;
			if(std::isnan(y))y = value.y;
			return *this;
		}

		FORCE_INLINE constexpr vector2& nan_to0() noexcept{
			if(std::isnan(x))x = 0;
			if(std::isnan(y))y = 0;
			return *this;
		}

		FORCE_INLINE constexpr vector2& nan_to1() noexcept{
			if(std::isnan(x))x = 1;
			if(std::isnan(y))y = 1;
			return *this;
		}

		FORCE_INLINE constexpr vector2& mod(const T ox, const T oy) noexcept {
			x = math::mod(x, ox);
			y = math::mod(y, oy);
			return *this;
		}

		FORCE_INLINE constexpr vector2& mod(const T val) noexcept {
			return this->mod(val, val);
		}

		FORCE_INLINE constexpr vector2& mod(const_pass_t other) noexcept {
			return this->mod(other.x, other.y);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 copy() const noexcept {
			return vector2{ x, y };
		}

		FORCE_INLINE constexpr vector2& set_zero() noexcept {
			return this->set(static_cast<T>(0), static_cast<T>(0));
		}

		FORCE_INLINE constexpr vector2& set_NaN() noexcept requires std::floating_point<T> {
			return set(std::numeric_limits<float>::signaling_NaN(), std::numeric_limits<float>::signaling_NaN());
		}


		[[nodiscard]] PURE_FN FORCE_INLINE constexpr std::size_t hash_value() const noexcept requires (sizeof(T) <= 8){
			static constexpr std::hash<std::size_t> hasher{};

			if constexpr (sizeof(T) == 8){
				const std::uint64_t a = std::bit_cast<std::uint64_t>(x);
				const std::uint64_t b = std::bit_cast<std::uint64_t>(y);
				return hasher(a ^ b << 31);
			}else if constexpr (sizeof(T) == 4){
				return hasher(std::bit_cast<std::size_t>(*this));
			}else if constexpr (sizeof(T) == 2){
				return hasher(std::bit_cast<std::uint32_t>(*this));
			}else if constexpr (sizeof(T) == 2){
				return hasher(std::bit_cast<std::uint16_t>(*this));
			}else{
				static_assert(false);
				// static constexpr std::hash<const std::span<std::uint8_t>> fallback{};
				// //TODO start life time as
				// return fallback(std::span{reinterpret_cast<const std::uint8_t*>(this), sizeof(T) * 2});
			}
		}


		FORCE_INLINE constexpr vector2& set(const T ox, const T oy) noexcept {
			this->x = ox;
			this->y = oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& snap_to(const_pass_t snap, const_pass_t offset = {}) noexcept {
			this->x = math::snap_to(x, snap.x, offset.x);
			this->y = math::snap_to(y, snap.y, offset.y);

			return *this;
		}

		FORCE_INLINE constexpr vector2& set(const T val) noexcept {
			return this->set(val, val);
		}

		FORCE_INLINE constexpr vector2& set(const_pass_t tgt) noexcept {
			return this->operator=(tgt);
		}

		FORCE_INLINE constexpr vector2& add(const T ox, const T oy) noexcept {
			x += ox;
			y += oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& swap_xy() noexcept{
			std::swap(x, y);
			return *this;
		}

		FORCE_INLINE constexpr vector2& add(const T val) noexcept {
			x += val;
			y += val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& add_x(const T val) noexcept {
			x += val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& add_y(const T val) noexcept {
			y += val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& set_x(const T val) noexcept {
			x = val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& set_y(const T val) noexcept {
			y = val;

			return *this;
		}

		FORCE_INLINE constexpr vector2& add(const_pass_t other) noexcept {
			return this->add(other.x, other.y);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE friend constexpr vector2 fma(vector2 a, vector2 b, vector2 c) noexcept {
			return {math::fma(a.x, b.x, c.x), math::fma(a.y, b.y, c.y)};
		}

		[[nodiscard]] PURE_FN FORCE_INLINE friend constexpr vector2 fma(vector2 a, T b, vector2 c) noexcept {
			return {math::fma(a.x, b, c.x), math::fma(a.y, b, c.y)};
		}

	private:
		/**
		 * @return self * mul + add
		 */
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 fma(const_pass_t mul, const_pass_t add) const noexcept {
			vector2 rst;
			rst.x = math::fma(x, mul.x, add.x);
			rst.y = math::fma(y, mul.y, add.y);

			return rst;
		}

		/**
		 * @return other * scale + self
		 */
		template <typename Prog>
		PURE_FN FORCE_INLINE constexpr vector2 fma(const_pass_t other, const Prog scale_to_other) const noexcept {
			vector2 rst;

			if constexpr (std::floating_point<T> && std::floating_point<Prog>){
				if !consteval{
					rst.x = std::fma(other.x, scale_to_other, x);
					rst.y = std::fma(other.y, scale_to_other, y);
					return rst;
				}
			}

			rst.x = x + scale_to_other * other.x;
			rst.y = y + scale_to_other * other.y;

			return rst;
		}

	public:
		FORCE_INLINE constexpr vector2& sub(const T ox, const T oy) noexcept {
			x -= ox;
			y -= oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& sub(const_pass_t other) noexcept {
			return this->sub(other.x, other.y);
		}

		FORCE_INLINE constexpr vector2& fdim(const_pass_t other) noexcept requires (std::floating_point<T>)  {
			x = math::fdim(x, other.x);
			y = math::fdim(y, other.y);
			return *this;
		}

		FORCE_INLINE constexpr vector2& sub(const_pass_t other, const T scale) noexcept {
			return this->sub(other.x * scale, other.y * scale);
		}

		FORCE_INLINE constexpr vector2& mul(const T ox, const T oy) noexcept {
			x *= ox;
			y *= oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& mul_x(const T ox) noexcept {
			x *= ox;

			return *this;
		}

		FORCE_INLINE constexpr vector2& mul_y(const T oy) noexcept {
			y *= oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& mul(const T val) noexcept {
			return this->mul(val, val);
		}

		FORCE_INLINE constexpr vector2& reverse() noexcept requires mo_yanxi::signed_number<T> {
			x = -x;
			y = -y;
			return *this;
		}

		FORCE_INLINE constexpr vector2& mul(const_pass_t other) noexcept {
			return this->mul(other.x, other.y);
		}

		FORCE_INLINE constexpr vector2& div(const T ox, const T oy) noexcept {
			x /= ox;
			y /= oy;

			return *this;
		}

		FORCE_INLINE constexpr vector2& div(const T val) noexcept {
			return this->div(val, val);
		}

		FORCE_INLINE constexpr vector2& div(const_pass_t other) noexcept {
			return this->div(other.x, other.y);
		}


		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vec2_products<T> get_products_with(const_pass_t tgt) const noexcept{
			return vec2_products<T>{this->dot(tgt), this->cross(tgt)};
		}

		/*
		[[nodiscard]] FORCE_INLINE constexpr T getX() const noexcept{
			return x;
		}

		[[nodiscard]] FORCE_INLINE constexpr T getY() const noexcept{
			return y;
		}

		FORCE_INLINE constexpr vector2& setX(const T ox) noexcept{
			this->x = ox;
			return *this;
		}

		FORCE_INLINE constexpr vector2& setY(const T oy) noexcept{
			this->y = oy;
			return *this;
		}*/

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T dst2(const_pass_t other) const noexcept {
			return math::dst2(x, y, other.x, other.y);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T dst(const_pass_t other) const noexcept{
			return math::dst(x, y, other.x, other.y);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T dst(const T tx, const T ty) const noexcept{
			return math::dst(x, y, tx, ty);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T dst2(const T tx, const T ty) const noexcept{
			return math::dst2(x, y, tx, ty);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool within(const_pass_t other, const T dst) const noexcept{
			return this->dst2(other) < dst * dst;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool is_NaN() const noexcept{
			if constexpr(std::floating_point<T>) {
				return std::isnan(x) || std::isnan(y);
			} else {
				return false;
			}
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool is_Inf() const noexcept{
			return math::isinf(x) || math::isinf(y);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr float length() const noexcept {
			return math::dst(x, y);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T length2() const noexcept {
			return x * x + y * y;
		}

		/**
		 * @brief
		 * @return angle in [-180 deg, 180 deg]
		 */
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr float angle_deg() const noexcept {
			return angle_rad() * math::rad_to_deg_v<floating_point_t>;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr floating_point_t angle_to_deg(const_pass_t where) const noexcept {
			return (where - *this).angle_deg();
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr floating_point_t angle_to_rad(const_pass_t where) const noexcept {
			return (where - *this).angle_rad();
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr floating_point_t angle_between_deg(const_pass_t other) const noexcept {
			return math::atan2(this->cross(other), this->dot(other)) * math::rad_to_deg_v<floating_point_t>;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr floating_point_t angle_between_rad(const_pass_t other) const noexcept {
			return math::atan2(this->cross(other), this->dot(other));
		}

		FORCE_INLINE constexpr vector2& normalize() noexcept {
			return div(length());
		}

		FORCE_INLINE constexpr vector2& normalize_or_zero() noexcept {
			const auto len = length();
			if(len > std::numeric_limits<floating_point_t>::epsilon()){
				return div(len);
			}else{
				return set_zero();
			}

		}

		FORCE_INLINE constexpr vector2& to_sign() noexcept{
			x = math::sign<T>(x);
			y = math::sign<T>(y);

			return *this;
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& rotate_rad(const Fp rad) noexcept{
			const auto [c, s] = math::cos_sin(rad);
			//  Matrix Multi
			//  cos rad		-sin rad	x    crx   -sry
			//	sin rad		 cos rad	y	 srx	cry
			return this->rotate(c, s);
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& rotate(const Fp cos, const Fp sin) noexcept{
			if constexpr(std::floating_point<T>) {
				return this->set(cos * x - sin * y, sin * x + cos * y);
			}else {
				return this->set(
					static_cast<T>(cos * static_cast<Fp>(x) - sin * static_cast<Fp>(y)),
					static_cast<T>(sin * static_cast<Fp>(x) + cos * static_cast<Fp>(y))
				);
			}
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& rotate_deg(const Fp degree) noexcept{
			return this->rotate_rad(degree * math::deg_to_rad_v<Fp>);
		}

		FORCE_INLINE constexpr vector2& lerp_inplace(const_pass_t tgt, const floating_point_t alpha) noexcept{
			return this->set(math::lerp(x, tgt.x, alpha), math::lerp(y, tgt.y, alpha));
		}

		FORCE_INLINE constexpr vector2& approach(const_pass_t target, const mo_yanxi::arithmetic auto alpha) noexcept{
			vector2 approach = target - *this;
			const auto alpha2 = alpha * alpha;
			const auto len2 = approach.dst2();
			if (len2 <= alpha){
				return this->set(target);
			}

			const auto scl = math::sqrt(static_cast<floating_point_t>(alpha2) / static_cast<floating_point_t>(len2));
			return this->operator=(this->fma(approach, scl));
		}

		template <std::floating_point Fp>
		[[nodiscard]] FORCE_INLINE static constexpr vector2 from_polar_rad(const Fp angRad, const T length = 1) noexcept{
			return {length * math::cos(angRad), length * math::sin(angRad)};
		}

		template <std::floating_point Fp>
		[[nodiscard]] FORCE_INLINE static constexpr vector2 from_polar_deg(const Fp angDeg, const T length = 1) noexcept{
			return vector2::from_polar_rad(angDeg * math::deg_to_rad_v<Fp>, length);
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& set_polar_deg(const Fp angDeg, const T length) noexcept{
			return vector2::set_polar_rad(angDeg * math::deg_to_rad_v<Fp>, length);
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& set_polar_deg(const Fp angDeg) noexcept{
			return vector2::set_polar_deg(angDeg, length());
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& set_polar_rad(const Fp angDeg, const T length) noexcept{
			return vector2::set(length * math::cos(angDeg), length * math::sin(angDeg));
		}

		template <std::floating_point Fp>
		FORCE_INLINE constexpr vector2& set_polar_rad(const Fp angDeg) noexcept{
			return vector2::set_polar_deg(angDeg, length());
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T dot(const_pass_t tgt) const noexcept{
			return x * tgt.x + y * tgt.y;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T cross(const_pass_t tgt) const noexcept{
			return x * tgt.y - y * tgt.x;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 cross(const T val_zAxis) const noexcept{
			return {y * val_zAxis, -x * val_zAxis};
		}

		FORCE_INLINE constexpr vector2& project(const_pass_t tgt) noexcept{
			float scl = this->dot(tgt);

			return this->set(tgt).mul(scl / tgt.length2());
		}

		FORCE_INLINE constexpr vector2& project_scaled(const_pass_t tgt) noexcept {
			const float scl = this->dot(tgt);

			return this->set(tgt.x * scl, tgt.y * scl);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr auto scalar_proj2(const_pass_t axis) const noexcept{
			const auto dot = this->dot(axis);
			return dot * dot / axis.length2();
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr auto scalar_proj(const_pass_t axis) const noexcept{
			return math::sqrt(this->scalar_proj2(axis));
		}

		FORCE_INLINE friend constexpr bool operator==(const vector2& lhs, const vector2& rhs) noexcept = default;

		FORCE_INLINE constexpr vector2& clamp_x(const T min, const T max) noexcept {
			x = math::clamp(x, min, max);
			return *this;
		}


		FORCE_INLINE constexpr vector2& clamp_xy(const_pass_t min, const_pass_t max) noexcept {
			x = math::clamp(x, min.x, max.x);
			y = math::clamp(y, min.y, max.y);
			return *this;
		}


		FORCE_INLINE constexpr vector2& clamp_xy_normalized() noexcept {
			x = math::clamp<T>(x, 0, 1);
			y = math::clamp<T>(y, 0, 1);
			return *this;
		}

		FORCE_INLINE constexpr vector2& clamp_y(const T min, const T max) noexcept {
			y = math::clamp(y, min, max);
			return *this;
		}

		FORCE_INLINE constexpr vector2& min_x(const T min) noexcept {
			x = math::min(x, min);
			return *this;
		}

		FORCE_INLINE constexpr vector2& min_y(const T min) noexcept {
			y = math::min(y, min);
			return *this;
		}

		FORCE_INLINE constexpr vector2& max_x(const T max) noexcept {
			x = math::max(x, max);
			return *this;
		}

		FORCE_INLINE constexpr vector2& max_y(const T max) noexcept {
			y = math::max(y, max);
			return *this;
		}


		FORCE_INLINE constexpr vector2& min(const_pass_t min) noexcept {
			x = math::min(x, min.x);
			y = math::min(y, min.y);
			return *this;
		}

		FORCE_INLINE constexpr vector2& max(const_pass_t max) noexcept {
			x = math::max(x, max.x);
			y = math::max(y, max.y);
			return *this;
		}

		// FORCE_INLINE constexpr vector2& clampNormalized() noexcept requires std::floating_point<T>{
		// 	return clamp_x(0, 1).clamp_y(0, 1);
		// }


		FORCE_INLINE constexpr vector2& to_abs() noexcept {
			if constexpr(!std::is_unsigned_v<T>) {
				x = math::abs(x);
				y = math::abs(y);
			}

			return *this;
		}

		FORCE_INLINE constexpr vector2& limit_y(const T yAbs) {
			y = math::clamp_range(y, yAbs);
			return *this;
		}

		FORCE_INLINE constexpr vector2& limit_x(const T xAbs) {
			x = math::clamp_range(x, xAbs);
			return *this;
		}

		FORCE_INLINE constexpr vector2& limit(const T xAbs, const T yAbs) noexcept {
			this->limit_x(xAbs);
			this->limit_y(yAbs);
			return *this;
		}

		FORCE_INLINE constexpr vector2& limit_max_length(const T limit) noexcept {
			return this->limit_max_length2(limit * limit);
		}

		FORCE_INLINE constexpr vector2& limit_max_length2(const T limit2) noexcept {
			const float len2 = length2();
			if (len2 == 0) [[unlikely]] return *this;

			if (len2 > limit2) {
				return this->scl(math::sqrt(static_cast<floating_point_t>(limit2) / static_cast<floating_point_t>(len2)));
			}

			return *this;
		}

		FORCE_INLINE constexpr vector2& limit_min_length(const T limit) noexcept {
			return this->limit_min_length2(limit * limit);
		}

		FORCE_INLINE constexpr vector2& limit_min_length2(const T limit2) noexcept {
			const float len2 = length2();
			if (len2 == 0) [[unlikely]] return *this;

			if (len2 < limit2) {
				return this->scl(math::sqrt(static_cast<float>(limit2) / static_cast<float>(len2)));
			}

			return *this;
		}

		FORCE_INLINE constexpr vector2& limit_length(const T min, const T max) noexcept {
			return this->limit_length2(min * min, max * max);
		}


		FORCE_INLINE constexpr vector2& limit_length2(const T min, const T max) noexcept {
			if (const float len2 = length2(); len2 < min) {
				return this->scl(math::sqrt(static_cast<float>(min) / static_cast<float>(len2)));
			}else if(len2 > max){
				return this->scl(math::sqrt(static_cast<float>(max) / static_cast<float>(len2)));
			}

			return *this;
		}

		template <std::floating_point V>
		FORCE_INLINE constexpr vector2& scl_round(const vector2<V> val) noexcept {
			V rstX = static_cast<V>(x) * val.x;
			V rstY = static_cast<V>(y) * val.y;
			return this->set(math::round<T>(rstX), math::round<T>(rstY));
		}

		template <std::floating_point V>
		FORCE_INLINE constexpr vector2& scl(const V val) noexcept {
			return this->scl(val, val);
		}

		template <std::floating_point V>
		FORCE_INLINE constexpr vector2& scl(const V ox, const V oy) noexcept {
			x = static_cast<T>(static_cast<V>(x) * ox);
			y = static_cast<T>(static_cast<V>(y) * oy);
			return *this;
		}

		template <std::integral V>
		FORCE_INLINE constexpr vector2& scl(const V val) noexcept {
			return this->scl(static_cast<floating_point_t>(val), static_cast<floating_point_t>(val));
		}

		template <std::integral V>
		FORCE_INLINE constexpr vector2& scl(const V ox, const V oy) noexcept {
			return this->scl(static_cast<floating_point_t>(ox), static_cast<floating_point_t>(oy));
		}

		FORCE_INLINE constexpr vector2& flip_x() noexcept {
			x = -x;
			return *this;
		}

		FORCE_INLINE constexpr vector2& flip_y() noexcept {
			y = -y;
			return *this;
		}

		FORCE_INLINE constexpr vector2& set_length(const T len) noexcept {
			return this->set_length2(len * len);
		}

		FORCE_INLINE constexpr vector2& set_length2(const T len2) noexcept {
			const float oldLen2 = length2();
			return oldLen2 == 0 || oldLen2 == len2 ? *this : this->scl(math::sqrt(len2 / oldLen2));  // NOLINT(clang-diagnostic-float-equal)
		}


		[[nodiscard]] PURE_FN FORCE_INLINE constexpr floating_point_t angle_rad() const noexcept{
			return math::atan2(static_cast<floating_point_t>(y), static_cast<floating_point_t>(x));
		}

		// [[nodiscard]] float angleExact() const noexcept{
		// 	return std::atan2(y, x)/* * math::DEGREES_TO_RADIANS*/;
		// }

		/**
		 * \brief clockwise rotation
		 */
		FORCE_INLINE constexpr vector2& rotate_rt_clockwise() noexcept requires (std::is_signed_v<T>) {
			return this->set(-y, x);
		}

		/**
		 * @brief
		 * @param signProv > 0: counterClockwise; < 0: clockWise; = 0: do nothing
		 * @return
		 */
		template <mo_yanxi::arithmetic S>
		FORCE_INLINE constexpr vector2& rotate_rt_with(const S signProv) noexcept requires (std::is_signed_v<T>) {
			const int sign = math::sign(signProv);
			if(sign){
				return this->set(y * sign, -x * sign);
			}else [[unlikely]]{
				return *this;
			}
		}

		/**
		 * \brief clockwise rotation
		 */
		[[deprecated]] FORCE_INLINE constexpr vector2& rotate_rt() noexcept requires (std::is_signed_v<T>) {
			return rotate_rt_clockwise();
		}

		/**
		 * \brief counterclockwise rotation
		 */
		FORCE_INLINE constexpr vector2& rotate_rt_counter_clockwise() noexcept requires (std::is_signed_v<T>) {
			return this->set(y, -x);
		}

		FORCE_INLINE vector2& round_to(const_pass_t other) noexcept requires (std::floating_point<T>){
			x = math::round<T>(x, other.x);
			y = math::round<T>(y, other.y);

			return *this;
		}

		FORCE_INLINE vector2& round_to(const T val) noexcept requires (std::floating_point<T>){
			x = math::round<T>(x, val);
			y = math::round<T>(y, val);

			return *this;
		}

		template <typename N>
		[[nodiscard]] PURE_FN FORCE_INLINE vector2<N> round(typename vector2<N>::const_pass_t other) const noexcept{
			vector2<N> tgt = as<N>();
			tgt.x = math::round<N>(static_cast<N>(x), other.x);
			tgt.y = math::round<N>(static_cast<N>(y), other.y);

			return tgt;
		}

		template <typename N>
		[[nodiscard]] PURE_FN FORCE_INLINE vector2<N> round(const N val) const noexcept{
			vector2<N> tgt = as<N>();
			tgt.x = math::round<N>(static_cast<N>(x), val);
			tgt.y = math::round<N>(static_cast<N>(y), val);

			return tgt;
		}


		template <typename N>
		[[nodiscard]] PURE_FN FORCE_INLINE vector2<N> round() const noexcept requires (std::floating_point<T>){
			vector2<N> tgt = as<N>();
			tgt.x = math::round<N>(x);
			tgt.y = math::round<N>(y);

			return tgt;
		}

		FORCE_INLINE vector2& floor()  noexcept{
			if constexpr (std::floating_point<T>){
				x = std::floor(x);
				y = std::floor(y);
			}

			return *this;
		}

		FORCE_INLINE vector2& floor(T stride) noexcept{
			x = std::floor(x / stride) * stride;
			y = std::floor(y / stride) * stride;

			return *this;
		}

		FORCE_INLINE vector2& ceil(T stride) noexcept{
			x = std::ceil(x / stride) * stride;
			y = std::ceil(y / stride) * stride;

			return *this;
		}

		FORCE_INLINE vector2& ceil()  noexcept{
			if constexpr (std::floating_point<T>){
				x = std::ceil(x);
				y = std::ceil(y);
			}

			return *this;
		}

		FORCE_INLINE vector2& trunc() noexcept{
			if constexpr (std::floating_point<T>){
				x = math::trunc(x);
				y = math::trunc(y);
			}
			return *this;
		}

		FORCE_INLINE vector2& trunc(T step) noexcept{
			x = math::trunc(x, step);
			y = math::trunc(y, step);

			return *this;
		}


		FORCE_INLINE vector2& trunc(const_pass_t step) noexcept{
			x = math::trunc(x, step.x);
			y = math::trunc(y, step.y);

			return *this;
		}


		[[nodiscard]] PURE_FN FORCE_INLINE constexpr auto area() const noexcept {
			return x * y;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool is_zero() const noexcept {
			return x == static_cast<T>(0) && y == static_cast<T>(0);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool is_zero(const T margin) const noexcept{
			return length2() < margin;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 sign_or_zero() const noexcept{
			return {math::sign(x), math::sign(y)};
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2 sign() const noexcept{
			if constexpr (std::is_unsigned_v<T>){
				return {T{1}, T{1}};
			}
			return {static_cast<T>(x >= 0 ? 1 : -1), static_cast<T>(y >= 0 ? 1 : -1)};
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool within(const_pass_t other_in_axis) const noexcept{
			return x < other_in_axis.x && y < other_in_axis.y;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool beyond(const_pass_t other_in_axis) const noexcept{
			return x > other_in_axis.x || y > other_in_axis.y;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool within_equal(const_pass_t other_in_axis) const noexcept{
			return x <= other_in_axis.x && y <= other_in_axis.y;
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool beyond_equal(const_pass_t other_in_axis) const noexcept{
			return x >= other_in_axis.x || y >= other_in_axis.y;
		}

		[[nodiscard]] PURE_FN constexpr vector2 ortho_dir() const noexcept requires (mo_yanxi::signed_number<T>){
			static constexpr T Zero = static_cast<T>(0);
			static constexpr T One = static_cast<T>(1);
			static constexpr T NOne = static_cast<T>(-1);
			if(x >= Zero && y >= Zero){
				const bool onX = x > y;
				return {
					onX ? One : Zero,
					onX ? Zero : One,
				};
			}

			if(x <= Zero && y <= Zero){
				const bool onX = x < y;
				return {
					onX ? NOne : Zero,
					onX ? Zero : NOne,
				};
			}

			if(x > Zero && y < Zero){
				const bool onX = x > -y;
				return {
					onX ? One : Zero,
					onX ? Zero : NOne,
				};
			}

			if(x < Zero && y > Zero){
				const bool onX = -x > y;
				return {
					onX ? NOne : Zero,
					onX ? Zero : One,
				};
			}

			return {};
		}

		constexpr auto operator<=>(const vector2&) const = default;
		// auto constexpr operator<=>(ConstPassType v) const noexcept {
		// 	T len = length2();
		// 	T lenO = v.length2();
		//
		// 	if(len > lenO) {
		// 		return std::weak_ordering::greater;
		// 	}
		//
		// 	if(len < lenO) {
		// 		return std::weak_ordering::less;
		// 	}
		//
		// 	return std::weak_ordering::equivalent;
		// }

		[[nodiscard]] PURE_FN FORCE_INLINE bool within(const vector2 src, const vector2 dst) const noexcept{
			return x >= src.x && y >= src.y && x < dst.x && y < dst.y;
		}

		template <mo_yanxi::arithmetic TN>
		[[nodiscard]] PURE_FN  FORCE_INLINE constexpr vector2<TN> as() const noexcept{
			if constexpr (std::same_as<TN, T>){
				return *this;
			}else{
				return vector2<TN>{static_cast<TN>(x), static_cast<TN>(y)};
			}
		}

		template <std::signed_integral TN>
		PURE_FN constexpr explicit(false) operator vector2<TN>() const noexcept requires(std::signed_integral<T> && sizeof(TN) >= sizeof(T)){
			return as<TN>();
		}

		template <std::floating_point TN>
		PURE_FN constexpr explicit(false) operator vector2<TN>() const noexcept requires(std::floating_point<T> && sizeof(TN) >= sizeof(T)){
			return as<TN>();
		}

		template <std::unsigned_integral TN>
		PURE_FN constexpr explicit(false) operator vector2<TN>() const noexcept requires(std::unsigned_integral<T> && sizeof(TN) >= sizeof(T)){
			return as<TN>();
		}

		template <spec_of<vector2> TN>
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr auto as() const noexcept{
			return as<typename TN::value_type>();
		}

		template <mo_yanxi::arithmetic TN>
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr explicit operator vector2<TN>() const noexcept{
			return as<TN>();
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr auto as_signed() const noexcept{
			if constexpr (std::is_unsigned_v<T>){
				using S = std::make_signed_t<T>;
				return vector2<S>{static_cast<S>(x), static_cast<S>(y)};
			}else{
				return *this;
			}
		}

		friend std::ostream& operator<<(std::ostream& os, const_pass_t obj) {
			return os << '(' << std::to_string(obj.x) << ", " << std::to_string(obj.y) << ')';
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool equals(const_pass_t other) const noexcept{
			if constexpr (std::is_integral_v<T>){
				return *this == other;
			}else{
				return this->within(other, std::numeric_limits<T>::epsilon());
			}
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool equals(const_pass_t other, const T margin) const noexcept{
			return this->within(other, margin);
		}

		/**
		 * @brief
		 * @return y / x
		 */
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T slope() const noexcept{
			return y / x;
		}

		[[nodiscard]] FORCE_INLINE constexpr vector2& uniform() noexcept{
			x = x * 2 - 1;
			y = y * 2 - 1;
			return *this;
		}

		/**
		 * @brief
		 * @return x / y
		 */
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr T slope_inv() const noexcept{
			return x / y;
		}

		constexpr auto operator<=>(const vector2&) const noexcept requires (std::integral<T>) = default;

		template <typename Ty>
			requires std::constructible_from<Ty, T, T>
		PURE_FN FORCE_INLINE explicit constexpr operator Ty() const noexcept{
			return Ty(x, y);
		}

		template <typename N1, typename N2>
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool axis_less(const N1 ox, const N2 oy) const noexcept{
			if constexpr(std::floating_point<N1> || std::floating_point<N2> || std::floating_point<T>){
				using cmt = std::common_type_t<T, N1, N2>;

				return static_cast<cmt>(x) < static_cast<cmt>(ox) && static_cast<cmt>(y) < static_cast<cmt>(oy);
			} else{
				return std::cmp_less(x, ox) && std::cmp_less(y, oy);
			}
		}

		template <typename N>
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool axis_less(const vector2<N> other) const noexcept{
			return this->axis_less(other.x, other.y);
		}

		template <typename N1, typename N2>
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool axis_greater(const N1 ox, const N2 oy) const noexcept{
			if constexpr(std::floating_point<N1> || std::floating_point<N2> || std::floating_point<T>){
				using cmt = std::common_type_t<T, N1, N2>;

				return static_cast<cmt>(x) > static_cast<cmt>(ox) && static_cast<cmt>(y) > static_cast<cmt>(oy);
			} else{
				return std::cmp_greater(x, ox) && std::cmp_greater(y, oy);
			}
		}

		template <typename N>
		[[nodiscard]] PURE_FN FORCE_INLINE constexpr bool axis_greater(const vector2<N>& other) const noexcept{
			return this->axis_greater(other.x, other.y);
		}


		FORCE_INLINE friend constexpr value_type distance(const vector2& lhs, const vector2& rhs) noexcept{
			return lhs.dst(rhs);
		}

		FORCE_INLINE friend constexpr value_type abs(const vector2& v) noexcept{
			return v.length();
		}

		FORCE_INLINE friend constexpr value_type sqr(const vector2& v) noexcept{
			return v.length2();
		}
		FORCE_INLINE friend constexpr value_type dot(const vector2& l, const vector2& r) noexcept{
			return l.dot(r);
		}

	};

	export
	template <std::integral L, std::integral R>
	FORCE_INLINE constexpr vector2<bool> operator&&(const vector2<L>& l, const vector2<R>& r) noexcept{
		return {l.x && r.x, l.y && r.y};
	}

	export
	template <std::integral L, std::integral R>
	FORCE_INLINE constexpr vector2<bool> operator||(const vector2<L>& l, const vector2<R>& r) noexcept{
		return {l.x || r.x, l.y || r.y};
	}


	export using vec2 = vector2<float>;
	export using ivec2 = vector2<int>;
	export using bool2 = vector2<bool>;
	 /**
	  * @brief indicate a normalized vector2<float>
	  */
	export using nor_vec2 = vector2<float>;

	export using point2 = vector2<int>;
	export using i32point2 = vector2<std::int32_t>;
	export using isize2 = vector2<int>;
	export using usize2 = vector2<unsigned int>;
	export using upoint2 = vector2<unsigned int>;
	export using short_point2 = vector2<short>;
	export using ushort_point2 = vector2<unsigned short>;
	export using u32size2 = vector2<std::uint32_t>;
	export using u32point2 = vector2<std::uint32_t>;

	export
	template <std::floating_point T>
	struct optional_vec2 : vector2<T>{
		explicit constexpr operator bool() const noexcept{
			return !this->is_NaN();
		}

		constexpr void reset() noexcept{
			this->set_NaN();
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2<T> value_or(vector2<T>::const_pass_t v) const noexcept{
			if(*this){
				return *this;
			}else{
				return v;
			}
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr vector2<T> value_or() const noexcept{
			if(*this){
				return *this;
			}else{
				return {};
			}
		}

		using vector2<T>::operator=;



	};


	export namespace vectors{
		template <typename N = float>
		vector2<N> hit_normal(const vector2<N> approach_direction, const vector2<N> tangent){
			auto p = std::copysign(1, tangent.cross(approach_direction));
			return tangent.cross(p);
		}

		constexpr vec2 direction_deg(const float angle_Degree) noexcept{
			return {math::cos_deg(angle_Degree), math::sin_deg(angle_Degree)};
		}

		/*constexpr*/
		/**
		 * @brief
		 * @return snapped end
		 */
		constexpr vec2 snap_segment_to_angle(const vec2 src, const vec2 end, float angle_in_rad) noexcept{
			if(angle_in_rad == 0.0f) return end;

			vec2 delta = end - src;

			float originalAngle = delta.angle_rad();


			float snappedAngle = math::round(originalAngle, angle_in_rad);

			auto dir = vec2::from_polar_rad(snappedAngle);

			float projectionLength = dir.dot(delta);
			return dir * projectionLength + src;
		}

		template <typename T>
		struct constant2{
			static constexpr vector2<T> base_vec2{1, 1};
			static constexpr vector2<T> zero_vec2{0, 0};
			static constexpr vector2<T> x_vec2{1, 0};
			static constexpr vector2<T> y_vec2{0, 1};

			// static constexpr Vector2D<T> left_vec2{-1, 0};
			// static constexpr Vector2D<T> right_vec2{1, 0};
			// static constexpr Vector2D<T> up_vec2{0, 1};
			// static constexpr Vector2D<T> down_vec2{0, -1};
			//
			// static constexpr Vector2D<T> front_vec2{1, 0};
			// static constexpr Vector2D<T> back_vec2{-1, 0};

			static constexpr vector2<T> inf_positive_vec2{+std::numeric_limits<T>::infinity(), +std::numeric_limits<T>::infinity()};

			static constexpr vector2<T> max_vec2{std::numeric_limits<T>::max(), std::numeric_limits<T>::max()};
			static constexpr vector2<T> min_vec2{std::numeric_limits<T>::min(), std::numeric_limits<T>::min()};
			static constexpr vector2<T> lowest_vec2{std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest()};

			static constexpr vector2<T> SNaN{std::numeric_limits<T>::signaling_NaN(), std::numeric_limits<T>::signaling_NaN()};
			static constexpr vector2<T> QNaN_vec2{std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::quiet_NaN()};
		};
	}


	export
	template <std::floating_point T = float>
	constexpr inline optional_vec2<T> nullopt_vec2 = optional_vec2<T>{vectors::constant2<T>::SNaN};

}

// export
template <typename T>
struct std::hash<mo_yanxi::math::vector2<T>>{
	static std::size_t operator()(mo_yanxi::math::vector2<T>::const_pass_t v) noexcept{
		return v.hash_value();
	}
};

export
template <>
struct std::hash<mo_yanxi::math::vector2<int>>{
	static std::size_t operator()(mo_yanxi::math::vector2<int>::const_pass_t v) noexcept{
		return v.hash_value();
	}
};

export
template <>
struct std::hash<mo_yanxi::math::vector2<unsigned>>{
	static std::size_t operator()(mo_yanxi::math::vector2<unsigned>::const_pass_t v) noexcept{
		return v.hash_value();
	}
};

// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::vec2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::vec2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};
//
// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::point2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::point2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};
//
// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::upoint2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::upoint2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};
//
// export
// 	template<>
// 	struct std::hash<mo_yanxi::math::ushort_point2>{
// 		constexpr std::size_t operator()(const mo_yanxi::math::ushort_point2& v) const noexcept {
// 			return v.hash_value();
// 		}
// 	};



template <typename T>
struct formatter_base{
	static constexpr std::array wrapper{
		std::pair{std::string_view{"("}, std::string_view{")"}},
		std::pair{std::string_view{"["}, std::string_view{"]"}},
		std::pair{std::string_view{"{"}, std::string_view{"}"}},
	};

	std::size_t wrapperIndex{};
	std::formatter<T> formatter{};

	template <typename CharT>
	constexpr auto parse(std::basic_format_parse_context<CharT>& context) {
		auto begin = context.begin();
		switch(*begin){
			case ')' : [[fallthrough]];
			case '(' :{
				wrapperIndex = 0;
				++begin;
				break;
			}

			case '[' : [[fallthrough]];
			case ']' :{
				wrapperIndex = 1;
				++begin;
				break;
			}
			//
			// case '{' : [[fallthrough]];
			// case '}' :{
			// 	wrapperIndex = 2;
			// 	++begin;
			// 	break;
			// }

			case '!' :{
				wrapperIndex = wrapper.size();
				++begin;
				break;
			}

			default: break;
		}

		context.advance_to(begin);
		return formatter.parse(context);
	}

	template <typename OutputIt, typename CharT>
	auto format(mo_yanxi::math::vector2<T>::const_pass_t p, std::basic_format_context<OutputIt, CharT>& context) const{
		if(wrapperIndex != wrapper.size())std::format_to(context.out(), "{}", wrapper[wrapperIndex].first);
		formatter.format(p.x, context);
		std::format_to(context.out(), ", ");
		formatter.format(p.y, context);
		if(wrapperIndex != wrapper.size())std::format_to(context.out(), "{}", wrapper[wrapperIndex].second);
		return context.out();
	}
};

template <typename T>
struct std::formatter<mo_yanxi::math::vector2<T>>: formatter_base<T>{

};

// #define FMT(type) export template <> ;
//
// FMT(float);
// FMT(int);
// FMT(unsigned);
// FMT(std::size_t);
// FMT(bool);




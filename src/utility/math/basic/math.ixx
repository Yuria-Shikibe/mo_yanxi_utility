module;

#include "../../../../include/mo_yanxi/adapted_attributes.hpp"
#include <cassert>

#define MATH_ATTR CONST_FN FORCE_INLINE inline
#define MATH_ASSERT(expr) CHECKED_ASSUME(expr)

export module mo_yanxi.math;

import std;
import mo_yanxi.concepts;

//TODO clean up

namespace mo_yanxi::math{
namespace cpo_abs{
template <class T>
void abs(const T&) noexcept = delete;

struct impl{
	template <arithmetic T>
	[[nodiscard]] CONST_FN FORCE_INLINE static constexpr T operator()(T v) noexcept{
		if constexpr(std::unsigned_integral<T>) return v;
		if consteval{
			return v < 0 ? -v : v;
		}
		return std::abs(v);
	}

	template <typename T>
		requires requires(const T& v){
			requires !arithmetic<T>;
			{ abs(v) } noexcept -> arithmetic;
		}
	[[nodiscard]] CONST_FN FORCE_INLINE static constexpr auto operator()(const T& val) noexcept{
		return abs(val);
	}
};
}

namespace cpo_sqr{
template <class T>
void sqr(const T&) noexcept = delete;

struct impl{
	template <arithmetic T>
	[[nodiscard]] CONST_FN FORCE_INLINE static constexpr T operator()(T v) noexcept{
		return v * v;
	}

	template <typename T>
		requires requires(const T& v){
			requires !arithmetic<T>;
			{ sqr(v) } -> arithmetic;
		}
	[[nodiscard]] CONST_FN FORCE_INLINE static constexpr auto operator()(const T& val) noexcept{
		return sqr(val);
	}
};
}

namespace cpo_fma{
template <class T>
void fma(const T&) noexcept = delete;

struct impl{
	template <typename T1, typename T2, typename T3>
		requires requires(T1 v1, T2 v2, T3 v3){
			{ fma(v1, v2, v3) } noexcept -> std::convertible_to<T1>;
		}
	[[nodiscard]] CONST_FN FORCE_INLINE static constexpr auto operator()(
		const T1& a, const T2& b, const T3& c) noexcept{
		return fma(a, b, c);
	}

	template <typename T1, typename T2, typename T3>
	[[nodiscard]] CONST_FN FORCE_INLINE static constexpr auto operator()(
		const T1& a, const T2& b, const T3& c) noexcept{
		return a * b + c;
	}


	template <std::floating_point T>
		requires requires(T v1, T v2, T v3){
			{ std::fma(v1, v2, v3) } noexcept -> std::convertible_to<T>;
		}
	[[nodiscard]] CONST_FN FORCE_INLINE static constexpr auto operator()(
		const T a, const T b, const T c) noexcept{
		if consteval{
			return a * b + c;
		}
		return std::fma(a, b, c);
	}
};
}

inline namespace cpo{
export inline constexpr cpo_abs::impl abs;
export inline constexpr cpo_sqr::impl sqr;
export inline constexpr cpo_fma::impl fma;
}


/**
 * \return a correCt dst value safe for unsigned numbers, can be negative if params are signed.
 */
export
template <arithmetic T>
MATH_ATTR T constexpr dst_safe(const T src, const T dst) noexcept{
	if constexpr(!std::is_unsigned_v<T>){
		return dst - src;
	} else{
		return src > dst ? src - dst : dst - src;
	}
}

export
template <arithmetic T>
MATH_ATTR T constexpr dst_abs(const T src, const T dst) noexcept{
	return src > dst ? src - dst : dst - src;
}

namespace cpo_distance{
template <class T>
void distance(const T&, const T&) noexcept = delete;

template <class T>
concept Use_ADL_distance = requires(const T& lhs, const T& rhs){
	{ distance(lhs, rhs) } -> arithmetic;
};

struct impl{
	template <arithmetic T>
	[[nodiscard]] FORCE_INLINE static constexpr T operator()(T lhs, T rhs) noexcept{
		if constexpr(std::unsigned_integral<T>){
			return math::dst_safe(lhs, rhs);
		} else{
			return math::abs(lhs - rhs);
		}
	}

	template <typename T>
		requires (!arithmetic<T>)
	[[nodiscard]] FORCE_INLINE static constexpr auto operator()(const T& lhs, const T& rhs) noexcept{
		if constexpr(Use_ADL_distance<T>){
			return distance(lhs, rhs);
		} else{
			return abs(lhs - rhs);
		}
	}
};
} // namespace _Swap

inline namespace cpo{
export inline constexpr cpo_distance::impl distance;
}
}

namespace mo_yanxi::math{

export constexpr float inline pi = std::numbers::pi_v<float>;

export
template <std::floating_point Fp>
constexpr float inline pi_2_v = std::numbers::pi_v<Fp> * static_cast<Fp>(2);
export
template <std::floating_point Fp>
constexpr float inline pi_half_v = std::numbers::pi_v<Fp> / static_cast<Fp>(2);

export constexpr inline float pi_half = pi_half_v<float>;
export constexpr inline float pi_2 = pi_2_v<float>;


export constexpr inline float e = std::numbers::e_v<float>;
export constexpr inline float sqrt2 = std::numbers::sqrt2_v<float>;
export constexpr inline float sqrt3 = std::numbers::sqrt3_v<float>;

export constexpr inline float circle_deg_full = 360.0f;
export constexpr inline float circle_deg_half = 180.0f;

export
template <std::floating_point Fp>
constexpr Fp inline rad_to_deg_v = static_cast<Fp>(180) / std::numbers::pi_v<Fp>;

export
template <std::floating_point Fp>
constexpr Fp inline deg_to_rad_v = std::numbers::pi_v<Fp> / static_cast<Fp>(180);

export constexpr float inline rad_to_deg = rad_to_deg_v<float>;
export constexpr float inline deg_to_rad = deg_to_rad_v<float>;

constexpr inline int BIG_ENOUGH_INT = 16 * 1024;
constexpr inline double BIG_ENOUGH_FLOOR = BIG_ENOUGH_INT;
constexpr inline double CEIL = 0.99999;
constexpr inline double BIG_ENOUGH_ROUND = static_cast<double>(BIG_ENOUGH_INT) + 0.5f;

/**
 * @brief indicate a floating point generally in [0, 1]
 */
export using progress_t = float;

export
template <typename T>
struct cos_sin_result{
	T cos;
	T sin;
};

template <std::floating_point T>
constexpr inline T inv_two_pi = 1 / pi_2_v<T>;
/**
 * 将角度规约到 [-pi, pi] 区间
 * 注意：使用了简单的算术取整，追求速度而非极高精度
 */
template <std::floating_point T>
MATH_ATTR constexpr T reduce_angle(T x) noexcept {
    if (x >= -std::numbers::pi_v<T> && x <= std::numbers::pi_v<T>) {
        return x;
    }

    T quotient = static_cast<long long>(x * inv_two_pi<T> + (x >= 0 ? 0.5 : -0.5));
    return x - quotient * pi_2_v<T>;
}

template <std::floating_point T>
MATH_ATTR constexpr T poly_sin_impl(T x2) noexcept {
    return T(1.0) + x2 * (T(-0.166666546) + x2 * (T(0.00833216076) + x2 * T(-0.000195152959)));
}

template <std::floating_point T>
MATH_ATTR constexpr T poly_cos_impl(T x2) noexcept {
    return T(1.0) + x2 * (T(-0.5) + x2 * (T(0.0416664568) + x2 * (T(-0.00138873163) + x2 * T(0.0000244331571))));
}

export
template <std::floating_point T>
MATH_ATTR constexpr T sin(T x) noexcept {
    T x_reduced = math::reduce_angle(x);
    T x2 = x_reduced * x_reduced;
    return x_reduced * math::poly_sin_impl(x2);
}

export
template <std::floating_point T>
MATH_ATTR constexpr T cos(T x) noexcept {
    T x_reduced = math::reduce_angle(x);
    T x2 = x_reduced * x_reduced;
    return math::poly_cos_impl(x2);
}

export
template <std::floating_point T>
MATH_ATTR constexpr cos_sin_result<T> cos_sin(T x) noexcept {
    T x_reduced = math::reduce_angle(x);
    T x2 = x_reduced * x_reduced;

    T c = math::poly_cos_impl(x2);
    T s = x_reduced * math::poly_sin_impl(x2);

    return {c, s};
}

export
template <std::floating_point T>
MATH_ATTR cos_sin_result<T> cos_sin_exact(const T radians) noexcept{
	return {std::cos(radians), std::sin(radians)};
}

export MATH_ATTR constexpr float sin(const float radians, const float scl, const float mag) noexcept{
	return sin(radians / scl) * mag;
}


export MATH_ATTR constexpr float sin_deg(const float degrees) noexcept{
	return sin(degrees * deg_to_rad);
}

export MATH_ATTR constexpr float cos_deg(const float degrees) noexcept{
	return cos(degrees * deg_to_rad);
}

export
MATH_ATTR constexpr cos_sin_result<float> cos_sin_deg(const float degree) noexcept{
	return {cos_deg(degree), sin_deg(degree)};
}

/**
 * @return [0, max] within cycle
 */
export MATH_ATTR constexpr float absin(const float inRad, const float cycle, const float max) noexcept{
	return (sin(inRad, cycle / pi_2, max) + max) / 2.0f;
}

export MATH_ATTR constexpr float sin_range(const float inRad, const float cycle, const float min,
                                           const float max) noexcept{
	const auto sine = absin(inRad, cycle, max - min);
	return min + sine;
}

export MATH_ATTR constexpr float tan(const float radians) noexcept{
	return sin(radians) / cos(radians);
}

export MATH_ATTR constexpr float tan(const float radians, const float scl, const float mag) noexcept{
	return sin(radians / scl) / cos(radians / scl) * mag;
}

export MATH_ATTR constexpr float cos(const float radians, const float scl, const float mag) noexcept{
	return cos(radians / scl) * mag;
}

export
template <typename T>
MATH_ATTR constexpr T copysign(T val, T sgn) noexcept{
	if consteval{
		return sgn < 0 ? -val : val;
	} else{
		return std::copysign(val, sgn);
	}
}


/**
 * Returns true if the value is zero.
 * @param value N/A
 * @param tolerance represent an upper bound below which the value is considered zero.
 */

export
template <typename T>
MATH_ATTR constexpr bool zero(const T value, const T tolerance = std::numeric_limits<T>::epsilon()) noexcept{
	return abs(value) <= tolerance;
}


/**
 * Returns true if a is nearly equal to b. The function uses the default floating error tolerance.
 * @param a the first value.
 * @param b the second value.
 */
export
template <typename T, arithmetic V = T>
MATH_ATTR constexpr bool equal(const T& a, const T& b, const V& margin = std::numeric_limits<V>::epsilon()) noexcept{
	return math::zero(distance(a, b), margin);
}


namespace angles{
/** Wraps the given angle to the range [-pi, pi]
 * @param a the angle in radians
 * @return the given angle wrapped to the range [-pi, pi] */
export
template <std::floating_point T>
MATH_ATTR constexpr T uniform_rad(T rad) noexcept{
	if consteval{
		while(rad >= std::numbers::pi_v<T>){
			rad -= pi_2_v<T>;
		}
		while(rad < -std::numbers::pi_v<T>){
			rad += pi_2_v<T>;
		}
		return rad;
	} else{
		return std::remainder(rad, pi_2_v<T>);
	}
}

export
template <std::floating_point T>
MATH_ATTR constexpr T angle_distance(const T from, const T to){
	return angles::uniform_rad<T>(to - from);
}

export
template <std::floating_point T>
MATH_ATTR constexpr T angle_distance_abs(const T from, const T to){
	return abs(angles::uniform_rad<T>(to - from));
}
}


export
template <typename T>
	requires (std::is_arithmetic_v<T>)
MATH_ATTR constexpr T snap_to(T value, T step, T base_offset = {}) noexcept{
	if(step == T(0)){
		return value;
	}

	if constexpr(std::integral<T>){
		T halfStep = step / 2;
		T remainder = value % step;

		if(remainder <= halfStep){
			return value - remainder + base_offset;
		} else{
			return value + (step - remainder) + base_offset;
		}
	} else{
		return std::round(value / step) * step + base_offset;
	}
}

export
template <std::floating_point T1, std::floating_point T2>
[[deprecated]] MATH_ATTR auto angle_exact_positive(const T1 x, const T2 y) noexcept{
	using Ty = std::common_type_t<T1, T2>;
	auto result = std::atan2(static_cast<Ty>(y), static_cast<Ty>(x)) * rad_to_deg_v<Ty>;
	if(result < 0) result += static_cast<Ty>(circle_deg_full);
	return result;
}

/** A variant on atan that does not tolerate infinite inputs for speed reasons, and because infinite inputs
 * should never occur where this is used (only in {@link atan2(float, float)@endlink}).
 * @param i any finite float
 * @return an output from the inverse tangent function, from pi/-2.0 to pi/2.0 inclusive
 * */
export MATH_ATTR constexpr double atn(const double i) noexcept{
	// We use double precision internally, because some constants need double precision.
	const double n = abs(i);
	// c uses the "equally-good" formulation that permits n to be from 0 to almost infinity.
	const double c = (n - 1.0) / (n + 1.0);
	// The approximation needs 6 odd powers of c.
	const double c2 = c * c, c3 = c * c2, c5 = c3 * c2, c7 = c5 * c2, c9 = c7 * c2, c11 = c9 * c2;
	return
		copysign(std::numbers::pi * 0.25
		         + (0.99997726 * c - 0.33262347 * c3 + 0.19354346 * c5 - 0.11643287 * c7 + 0.05265332 * c9 -
			         0.0117212 * c11), i);
}

export
template <arithmetic T>
constexpr bool isinf(T val) noexcept{
	if constexpr(std::floating_point<T>){
		if consteval{
			return abs(val) > std::numeric_limits<T>::max();
		}
	}

	return std::isinf(val);
}

export
template <arithmetic T>
constexpr bool isnan(T val) noexcept{
	if constexpr(std::floating_point<T>){
		if consteval{
			return !(abs(val) <= std::numeric_limits<T>::infinity());
		}
	}

	return std::isnan(val);
}

/** Close approximation of the frequently-used trigonometric method atan2, with higher precision than libGDX's atan2
 * approximation. Average error is 1.057E-6 radians; maximum error is 1.922E-6. Takes y and x (in that unusual order) as
 * floats, and returns the angle from the origin to that point in radians. It is about 4 times faster than
 * {@link atan2(double, double)@endlink} (roughly 15 ns instead of roughly 60 ns for Math, on Java 8 HotSpot). <br>
 * Credit for this goes to the 1955 research study "Approximations for Digital Computers," by RAND Corporation. This is sheet
 * 11's algorithm, which is the fourth-fastest and fourth-least precise. The algorithms on sheets 8-10 are faster, but only by
 * a very small degree, and are considerably less precise. That study provides an atan method, and that cleanly
 * translates to atan2().
 * @param y y-component of the point to find the angle towards; note the parameter order is unusual by convention
 * @param x x-component of the point to find the angle towards; note the parameter order is unusual by convention
 * @return the angle to the given point, in radians as a float; ranges from [-pi to pi] */
export
template <std::floating_point T>
MATH_ATTR constexpr T atan2(const T y, T x) noexcept{
	T n = y / x;

	if(math::isnan(n)) [[unlikely]] {
		n = y == x ? static_cast<T>(1.0f) : static_cast<T>(-1.0f); // if both y and x are infinite, n would be NaN
	} else if(math::isinf(n)) [[unlikely]] {
		x = static_cast<T>(0.0f); // if n is infinite, y is infinitely larger than x.
	}


	if(x > 0.0f){
		return static_cast<T>(math::atn(n));
	}
	if(x < 0.0f){
		return y >= 0
			       ? static_cast<T>(math::atn(n)) + std::numbers::pi_v<T>
			       : static_cast<T>(math::atn(n)) - std::numbers::pi_v<T>;
	}
	if(y > 0.0f){
		return x + pi_half_v<T>;
	}
	if(y < 0.0f){
		return x - pi_half_v<T>;
	}

	return x + y; // returns 0 for 0,0 or NaN if either y or x is NaN
}

export
template <small_object T>
MATH_ATTR constexpr std::ranges::min_max_result<T> minmax(const T a, const T b) noexcept{
	if(b < a){
		return {b, a};
	}

	return {a, b};
}

export
template <std::floating_point T = float, typename IdxT>
[[nodiscard]] MATH_ATTR constexpr T idx_to_factor(IdxT idx, IdxT total) noexcept{
	assert(idx <= total);
	return static_cast<T>(idx) / static_cast<T>(total);
}

export
template <small_object T>
	requires requires(T t){
		{ t < t } -> std::convertible_to<bool>;
	}
MATH_ATTR constexpr T max(const T v1, const T v2) noexcept(noexcept(v2 < v1)){
	if constexpr(std::floating_point<T>){
		if !consteval{
			return std::fmax(v1, v2);
		}
	}

	return v2 < v1 ? v1 : v2;
}

export
template <small_object T>
	requires requires(T t){
		{ t < t } -> std::convertible_to<bool>;
	}
MATH_ATTR constexpr T min(const T v1, const T v2) noexcept(noexcept(v1 < v2)){
	if constexpr(std::floating_point<T>){
		if !consteval{
			return std::fmin(v1, v2);
		}
	}

	return v1 < v2 ? v1 : v2;
}


export
template <small_object T, typename... Args>
	requires (small_object<Args> && ...)
MATH_ATTR constexpr std::ranges::min_max_result<T> minmax(T first, T second, Args... args) noexcept{
	const auto [min1, max1] = math::minmax(first, second);

	if constexpr(sizeof ...(Args) == 1){
		return {math::min(min1, args...), math::max(max1, args...)};
	} else{
		const auto [min2, max2] = math::minmax(args...);
		return {math::min(min1, min2), math::max(max1, max2)};
	}
}

export
template <std::integral T>
	requires (sizeof(T) <= 4)
[[deprecated]] MATH_ATTR constexpr int digits(const T n_positive) noexcept{
	return n_positive < 100000
		       ? n_positive < 100
			         ? n_positive < 10
				           ? 1
				           : 2
			         : n_positive < 1000
			         ? 3
			         : n_positive < 10000
			         ? 4
			         : 5
		       : n_positive < 10000000
		       ? n_positive < 1000000
			         ? 6
			         : 7
		       : n_positive < 100000000
		       ? 8
		       : n_positive < 1000000000
		       ? 9
		       : 10;
}

export
template <mo_yanxi::arithmetic T>
[[deprecated]] MATH_ATTR int digits(const T n_positive) noexcept{
	return n_positive == 0 ? 1 : static_cast<int>(std::log10(n_positive) + 1);
}

export
template <std::floating_point T>
MATH_ATTR constexpr T fdim(T to_sub, T sub) noexcept{
	if consteval{
		return math::max<T>(to_sub - sub, 0);
	}else{
		return std::fdim(to_sub, sub);
	}
}

export
template <std::floating_point T>
constexpr T sqrt(const T x) noexcept{
	if consteval{
		if(x < T(0)) return std::numeric_limits<T>::quiet_NaN();
		if(x == T(0)) return T(0);
		if(x == T(1)) return T(1);
		if(x >= std::numeric_limits<T>::infinity()) return x;

		T curr;
		if constexpr(std::same_as<T, float>){
			auto i = std::bit_cast<std::int32_t>(x);
			i = 0x5F375A86ui32 - (i >> 1);
			curr = std::bit_cast<T>(i);
		} else if constexpr(std::same_as<T, double>){
			auto i = std::bit_cast<std::int64_t>(x);
			i = 0x5FE6EB50C7B537A9ui64 - (i >> 1);
			curr = std::bit_cast<T>(i);
		} else{
			curr = x / T(2);
		}

		T prev{};

		while(prev != curr){
			prev = curr;
			curr = (curr + x / curr) / T(2);
		}

		return curr;
	} else{
		return std::sqrt(x);
	}
}

export
template <typename T1, typename T2>
/*requires requires(T1 v1, T2 v2){
	v1 - v1;
	(v1 - v1) / (v1 - v1);

	v2 - v2;
	{ (v1 - v1) / (v1 - v1) * (v2 - v2) } -> std::convertible_to<T2>;
}*/
MATH_ATTR constexpr T2 map(
	const T1 value,
	const T1 from, const T1 to,
	const T2 mappedFrom, const T2 mappedTo
) noexcept/*nvm*/{
	const auto rng = (to - from);
	if(math::zero(rng)) return mappedFrom;
	const auto prog = (value - from) / rng;
	if constexpr(std::floating_point<T2> && std::floating_point<decltype(prog)>){
		return math::fma(prog, mappedTo - mappedFrom, mappedFrom);
	} else{
		return prog * (mappedTo - mappedFrom) + mappedFrom;
	}
}

/** Map value from [0, 1].*/
MATH_ATTR constexpr float map(const float value, const float from, const float to) noexcept{
	return map(value, 0.f, 1.f, from, to);
}

/**Returns -1 if f<0, 1 otherwise.*/
MATH_ATTR constexpr bool diff_sign(const arithmetic auto a, const arithmetic auto b) noexcept{
	return a * b < 0;
}

export
template <arithmetic T>
MATH_ATTR constexpr T sign(const T f) noexcept{
	if constexpr(std::floating_point<T>){
		if(f == static_cast<T>(0.0)) [[unlikely]] {
			return static_cast<T>(0.0);
		}

		return f < static_cast<T>(0.0) ? static_cast<T>(-1.0) : static_cast<T>(1.0);
	} else if constexpr(std::unsigned_integral<T>){
		return f == 0 ? 0 : 1;
	} else{
		return f == 0 ? 0 : f < 0 ? -1 : 1;
	}
}


export
template <unsigned Exponent, typename T>
MATH_ATTR constexpr T pow_integral(const T val) noexcept{
	if constexpr(Exponent == 0){
		return 1;
	} else if constexpr(Exponent == 1){
		return val;
	} else if constexpr(Exponent % 2 == 0){
		const T v = math::pow_integral<Exponent / 2, T>(val);
		return v * v;
	} else{
		const T v = math::pow_integral<(Exponent - 1) / 2, T>(val);
		return val * v * v;
	}
}

export
template <typename T, std::unsigned_integral E>
MATH_ATTR constexpr T pow_integral(T val, const E exp) noexcept{
	const auto count = sizeof(E) * 8 - std::countl_zero(exp);

	T rst{1};

	for(auto i = count; i; --i){
		rst *= rst;
		if(exp >> (i - 1) & static_cast<E>(1u)){
			rst *= val;
		}
	}

	return rst;
}

export
template <typename T, std::signed_integral E>
MATH_ATTR constexpr T pow_integral(T val, const E exp) noexcept{
	assert(exp > 0);
	return math::pow_integral(val, static_cast<std::make_unsigned_t<E>>(exp));
}

export
template <typename T, std::unsigned_integral E>
MATH_ATTR constexpr T div_integral(const T val, const T div, const E times) noexcept{
	T rst = val;
	for(E i = 0; i < times; ++i){
		rst /= div;
	}
	return rst;
}

export
template <std::integral T>
MATH_ATTR constexpr T div_ceil(const T val, const T div) noexcept{
	return (val + div - 1) / div;
}

/** Returns the next power of two. Returns the specified value if the value is already a power of two. */
export MATH_ATTR constexpr int next_bit_ceil(std::unsigned_integral auto value) noexcept{
	return std::bit_ceil(value) + 1;
}

export
template <mo_yanxi::arithmetic T>
MATH_ATTR constexpr T clamp(const T v, const T min = static_cast<T>(0), const T max = static_cast<T>(1)) noexcept{
	MATH_ASSERT(min <= max);
	return v < min ? min : v > max ? max : v;
}

export
template <mo_yanxi::arithmetic T>
MATH_ATTR constexpr T clamp_sorted(const T v, const T min, const T max) noexcept{
	const auto [l, r] = math::minmax(min, max);
	if(v > r) return max;
	if(v < l) return min;

	return v;
}


export
template <mo_yanxi::arithmetic T>
[[nodiscard]] MATH_ATTR constexpr T clamp_range(const T v, const T absMax) noexcept{
	MATH_ASSERT(absMax >= 0);

	if(math::abs(v) > absMax){
		return math::copysign(absMax, v);
	}

	return v;
}

export
template <mo_yanxi::arithmetic T>
MATH_ATTR constexpr T max_abs(const T v1, const T v2) noexcept{
	if constexpr(mo_yanxi::non_negative<T>){
		return math::max(v1, v2);
	} else{
		return math::abs(v1) > math::abs(v2) ? v1 : v2;
	}
}

export
template <mo_yanxi::arithmetic T>
MATH_ATTR constexpr T min_abs(const T v1, const T v2) noexcept{
	if constexpr(mo_yanxi::non_negative<T>){
		return math::min(v1, v2);
	} else{
		return math::abs(v1) < math::abs(v2) ? v1 : v2;
	}
}

export
template <mo_yanxi::arithmetic T>
MATH_ATTR constexpr T clamp_positive(const T val) noexcept{
	return math::max<T>(val, 0);
}


/** Approaches a value at linear speed. */
export MATH_ATTR constexpr auto approach(const auto from, const auto to, const auto speed) noexcept{
	return from + math::clamp<decltype(from)>(to - from, -speed, speed);
}

/** Approaches a value at linear speed. */
export FORCE_INLINE constexpr void approach_inplace(auto& from, const auto to, const auto speed) noexcept{
	from += math::clamp<std::remove_cvref_t<decltype(from)>>(to - from, -speed, speed);
}

/** Approaches a value at linear speed. */
export FORCE_INLINE constexpr auto approach_inplace_get_delta(auto& from, const auto to, const auto speed) noexcept{
	auto delta = math::clamp<std::remove_cvref_t<decltype(from)>>(to - from, -speed, speed);
	from += delta;
	return delta;
}

export
template <typename T>
struct approach_result{
	T rst;
	bool reached;

	CONST_FN FORCE_INLINE constexpr explicit operator bool() const noexcept{
		return reached;
	}

	CONST_FN FORCE_INLINE constexpr explicit(false) operator const T&() const noexcept{
		return rst;
	}
};

export
template <typename T>
MATH_ATTR constexpr approach_result<T> forward_approach_then(const T current, const T destination,
                                                             const T delta) noexcept{
	MATH_ASSERT(current <= destination);
	const auto rst = current + delta;
	if(rst >= destination){
		return {destination, true};
	} else{
		return {rst, false};
	}
}

export
template <typename T>
MATH_ATTR constexpr float forward_approach(const T current, const T destination, const float delta) noexcept{
	MATH_ASSERT(current <= destination);

	const auto rst = current + delta;
	if(rst >= destination){
		return destination;
	} else{
		return rst;
	}
}

namespace cpo_lerp{
template <typename T, typename Prog>
void lerp(const T&, const T&, Prog) noexcept = delete;

struct impl{
	template <arithmetic T, arithmetic Prog>
	[[nodiscard]] FORCE_INLINE static constexpr T operator()(const T fromValue, const T toValue,
	                                                         const Prog progress) noexcept{
		if constexpr(std::floating_point<T> && std::floating_point<Prog>){
			if !consteval{
				return math::fma((toValue - fromValue), progress, fromValue);
			}
		}

		return fromValue * (1 - progress) + toValue * progress;
	}

	template <typename T, typename Prog>
		requires (!arithmetic<T> && requires(const T& v, const Prog& prog){
			{ lerp(v, v, prog) } noexcept -> std::same_as<T>;
		})
	[[nodiscard]] FORCE_INLINE static constexpr T operator()(const T& fromValue, const T& toValue,
	                                                         const Prog& progress) noexcept{
		return lerp(fromValue, toValue, progress);
	}

	template <typename T, typename Prog>
		requires (!arithmetic<T> && !requires(const T& v, const Prog& prog){
			{ lerp(v, v, prog) } noexcept -> std::same_as<T>;
		})
	[[nodiscard]] FORCE_INLINE static constexpr T operator()(const T& fromValue, const T& toValue,
	                                                         const Prog& progress) noexcept{
		return fromValue + (toValue - fromValue) * progress;
	}
};
}

inline namespace cpo{
export constexpr inline cpo_lerp::impl lerp{};
}


namespace cpo_dot{
template <typename T, typename Prog>
void dot(const T&, const T&) noexcept = delete;

struct impl{
	template <arithmetic T>
	[[nodiscard]] FORCE_INLINE static constexpr T operator()(const T l, const T r) noexcept{
		return l * r;
	}

	template <std::convertible_to<float> T>
	[[nodiscard]] FORCE_INLINE static constexpr T operator()(const T& l, const T& r) noexcept{
		return l * r;
	}

	template <std::convertible_to<double> T>
	[[nodiscard]] FORCE_INLINE static constexpr T operator()(const T& l, const T& r) noexcept{
		return l * r;
	}

	template <typename T>
		requires (!arithmetic<T> && requires(const T& v){
			{ dot(v, v) } noexcept -> arithmetic;
		})
	[[nodiscard]] FORCE_INLINE static constexpr auto operator()(const T& fromValue, const T& toValue) noexcept{
		return dot(fromValue, toValue);
	}
};
}

inline namespace cpo{
export constexpr inline cpo_dot::impl dot{};
}

export
template <mo_yanxi::small_object T, typename Prog>
FORCE_INLINE constexpr void lerp_inplace(T& fromValue, const T toValue, const Prog progress) noexcept{
	if constexpr(std::floating_point<T> && std::floating_point<Prog>){
		if !consteval{
			fromValue = math::fma((toValue - fromValue), progress, fromValue);
			return;
		}
	}

	fromValue += (toValue - fromValue) * progress;
}

/**
 * TODO transition
 * Returns the largest integer less than or equal to the specified float. This method will only properly floor floats from
 * -(2^14) to (Float.MAX_VALUE - 2^14).
 */
export
template <std::integral T = int>
MATH_ATTR constexpr T floorLEqual(const float value) noexcept{
	return static_cast<T>(value + BIG_ENOUGH_FLOOR) - BIG_ENOUGH_INT;
}

export
template <arithmetic T>
MATH_ATTR constexpr T trunc(T value, T step) noexcept{
	if(step == 0) [[unlikely]] return value;

	if constexpr(std::floating_point<T>){
		return std::trunc(value / step) * step;
	} else{
		return (value / step) * step;
	}
}

export
template <arithmetic T>
MATH_ATTR constexpr T trunc(T value) noexcept{
	if constexpr(std::floating_point<T>){
		return std::trunc(value);
	} else{
		return value;
	}
}

export
template <std::floating_point T>
MATH_ATTR constexpr T frac(T value) noexcept{
	if consteval{
		return value - static_cast<long long>(value);
	} else{
		return value - std::trunc(value);
	}
}


/**
 * Returns the smallest integer greater than or equal to the specified float. This method will only properly ceil floats from
 * -(2^14) to (Float.MAX_VALUE - 2^14).
 */
export
template <typename T = int>
MATH_ATTR constexpr T ceil(const float value) noexcept{
	if consteval{
		return BIG_ENOUGH_INT - static_cast<T>(BIG_ENOUGH_FLOOR - value);
	} else{
		return static_cast<T>(std::ceil(value));
	}
}

export
template <std::integral T, typename T0>
MATH_ATTR constexpr T round(const T0 value) noexcept{
	if constexpr(std::floating_point<T0>){
		return std::lround(value);
	} else{
		return static_cast<T>(value);
	}
}

export MATH_ATTR constexpr auto floor(const std::integral auto value, const std::integral auto step) noexcept{
	return value / step * step;
}

export
template <std::integral T = int>
MATH_ATTR constexpr T floor(const float value, const T step = 1) noexcept{
	return static_cast<T>(value / static_cast<float>(step)) * step;
}

export
template <std::integral T = int>
MATH_ATTR constexpr T floor(const float value) noexcept{
	return static_cast<T>(value);
}

export
template <mo_yanxi::arithmetic T>
MATH_ATTR T round(const T num, const T step){
	if(step == 0){
		return num;
	}
	if constexpr(std::floating_point<T>){
		return static_cast<T>(std::round(num / step) * step);
	} else{
		return static_cast<T>(std::lround(
			std::round(static_cast<double>(num) / static_cast<double>(step)) * static_cast<double>(step)));
	}
}

/**
 * Returns true if a is nearly equal to b.
 * @param a the first value.
 * @param b the second value.
 * @param tolerance represent an upper bound below which the two values are considered equal.
 */
export MATH_ATTR constexpr bool equal(const float a, const float b, const float tolerance) noexcept{
	return abs(a - b) <= tolerance;
}

export
template <arithmetic T>
MATH_ATTR constexpr T mod(const T x, const T n) noexcept{
	if constexpr(std::floating_point<T>){
		if consteval{
			return x - static_cast<T>(static_cast<long long>(x / n)) * n;
		}
		return std::fmod(x, n);
	} else{
		return x % n;
	}
}

export
template <std::ranges::random_access_range Rng, std::floating_point V>
[[nodiscard]] MATH_ATTR constexpr std::ranges::range_value_t<Rng> lerp_span(const Rng& values, const V time) noexcept{
	assert(!std::ranges::empty(values));

	if(time >= 1){
		return *std::ranges::rbegin(values);
	}

	if(time <= 0){
		return *std::ranges::begin(values);
	}

	auto cur = time * static_cast<V>(std::ranges::size(values));
	V idxv;
	auto rem = std::modf(cur, &idxv);

	auto idx = static_cast<std::ranges::range_size_t<Rng>>(idxv);

	return lerp(values[idx], values[idx + std::ranges::range_size_t<Rng>{1}], rem);
}


export
template <typename T, std::size_t size>
MATH_ATTR constexpr float lerp_span(const std::array<T, size>& values, float time) noexcept{
	if constexpr(size == 0){
		return T{};
	}

	time = clamp(time);
	const auto sizeF = static_cast<float>(size - 1ull);
	const float pos = time * sizeF;

	const auto cur = static_cast<std::size_t>(pos);
	const auto next = math::min(cur + 1ULL, size - 1ULL);
	const float mod = pos - static_cast<float>(cur);
	return math::lerp(values[cur], values[next], mod);
}

export
/** @return the input 0-1 value scaled to 0-1-0. */
MATH_ATTR constexpr float slope(const float fin) noexcept{
	return 1.0f - abs(fin - 0.5f) * 2.0f;
}

/**Converts a 0-1 value to 0-1 when it is in [offset, 1].*/
export
template <std::floating_point Fp>
MATH_ATTR constexpr Fp curve(const Fp f, const Fp offset) noexcept{
	if(f < offset){
		return 0.0f;
	}
	return (f - offset) / (static_cast<Fp>(1.0f) - offset);
}

/**Converts a 0-1 value to 0-1 when it is in [offset, to].*/
export
template <std::floating_point Fp>
MATH_ATTR constexpr Fp curve(const Fp f, const Fp from, const Fp to) noexcept{
	MATH_ASSERT(from < to);

	if(f <= from){
		return 0.0f;
	}
	if(f >= to){
		return 1.0f;
	}
	return (f - from) / (to - from);
}

/** Transforms a 0-1 value to a value with a 0.5 plateau in the middle. When margin = 0.5, this method doesn't do anything. */
export MATH_ATTR constexpr float curve_margin(const float f, const float marginLeft, const float marginRight) noexcept{
	if(f < marginLeft) return f / marginLeft * 0.5f;
	if(f > 1.0f - marginRight) return (f - 1.0f + marginRight) / marginRight * 0.5f + 0.5f;
	return 0.5f;
}


/** Transforms a 0-1 value to a value with a 0.5 plateau in the middle. When margin = 0.5, this method doesn't do anything. */
export MATH_ATTR constexpr float curve_margin(const float f, const float margin) noexcept{
	return curve_margin(f, margin, margin);
}

export
template <typename T>
MATH_ATTR constexpr T dst(const T x1, const T y1) noexcept{
	if consteval{
		return static_cast<T>(math::sqrt<double>(x1 * x1 + y1 * y1));
	} else{
		return static_cast<T>(std::hypot(x1, y1));
	}
}

export
template <typename T>
MATH_ATTR constexpr T dst2(const T x1, const T y1) noexcept{
	return x1 * x1 + y1 * y1;
}

export
template <typename T>
MATH_ATTR constexpr T dst(const T x1, const T y1, const T x2, const T y2) noexcept{
	return math::dst(math::dst_safe(x2, x1), math::dst_safe(y2, y1));
}

export
template <typename T>
MATH_ATTR constexpr T dst2(const T x1, const T y1, const T x2, const T y2) noexcept{
	const float xd = math::dst_safe(x1, x2);
	const float yd = math::dst_safe(y1, y2);
	return xd * xd + yd * yd;
}


/** Manhattan distance. */
export
template <typename T>
MATH_ATTR constexpr T dst_mht(const T x1, const T y1, const T x2, const T y2) noexcept{
	return math::dst_abs(x1, x2) + math::dst_abs(y1, y2);
}


/** @return whether dst(x1, y1, x2, y2) < dst */
export
template <typename T>
MATH_ATTR constexpr bool within(const T x1, const T y1, const T x2, const T y2, const T dst) noexcept{
	return math::dst2(x1, y1, x2, y2) < dst * dst;
}

/** @return whether dst(x, y, 0, 0) < dst */
export
template <typename T>
MATH_ATTR constexpr bool within(const T x1, const T y1, const T dst) noexcept{
	return (x1 * x1 + y1 * y1) < dst * dst;
}


export
template <typename T>
struct section{
	T from;
	T to;

	MATH_ATTR constexpr bool within_closed(const T& value) const noexcept requires std::totally_ordered<T>{
		return value >= from && value <= to;
	}

	MATH_ATTR constexpr bool within_open(const T& value) const noexcept requires std::totally_ordered<T>{
		return value > from && value < to;
	}

	MATH_ATTR constexpr bool within_lo_rc(const T& value) const noexcept requires std::totally_ordered<T>{
		return value > from && value <= to;
	}

	MATH_ATTR constexpr bool within_lc_ro(const T& value) const noexcept requires std::totally_ordered<T>{
		return value >= from && value < to;
	}

	MATH_ATTR constexpr section get_ordered() const noexcept{
		const auto [min, max] = std::minmax(from, to);
		return {min, max};
	}

	MATH_ATTR constexpr section& expand(const T& value) noexcept{
		from -= value;
		to += value;
		return *this;
	}

	MATH_ATTR constexpr T normalize(const T value) const noexcept requires (std::floating_point<T>){
		return math::curve<T>(value, from, to);
	}

	template <std::floating_point Tgt = float>
	MATH_ATTR constexpr Tgt normalize(const T value) const noexcept requires (!std::floating_point<T>){
		return math::curve<Tgt>(static_cast<Tgt>(value), static_cast<Tgt>(from), static_cast<Tgt>(to));
	}

	MATH_ATTR constexpr T length() const noexcept{
		return math::dst_safe(from, to);
	}

	MATH_ATTR constexpr T clamp(const T val) const noexcept{
		return math::clamp(val, from, to);
	}

	MATH_ATTR constexpr T operator[](std::floating_point auto fp) const noexcept{
		return math::lerp(from, to, fp);
	}

	FORCE_INLINE constexpr section& operator*=(const T val) noexcept{
		from *= val;
		to *= val;
		return *this;
	}

	FORCE_INLINE constexpr friend section operator*(section section, const T val) noexcept{
		section *= val;
		return section;
	}

	FORCE_INLINE constexpr friend section operator*(const T val, section section) noexcept{
		section *= val;
		return section;
	}

	FORCE_INLINE constexpr section& operator/=(const T val) noexcept{
		from /= val;
		to /= val;
		return *this;
	}

	FORCE_INLINE constexpr friend section operator/(section section, const T val) noexcept{
		section /= val;
		return section;
	}

	//
	// FORCE_INLINE constexpr friend section operator/(const T val, section section) noexcept{
	// 	section /= val;
	// 	return section;
	// }
};

export
using range = section<float>;

export
template <std::floating_point T>
constexpr inline section<T> full_circle_rad{-std::numbers::pi_v<T>, std::numbers::pi_v<T>};

export
template <std::floating_point T>
constexpr inline section<T> full_circle_deg{-180, 180};

export
template <typename T>
struct based_section{
	T base;
	T extent;

	MATH_ATTR constexpr T length() const noexcept{
		return math::abs(extent);
	}

	MATH_ATTR constexpr T operator[](std::floating_point auto fp) const noexcept{
		return math::fma(fp, extent, base);
	}

	operator section<T>(){
		return section<T>{base, base + extent};
	}
};

export
template <std::floating_point T>
[[deprecated]] MATH_ATTR constexpr T snap(T value, T step){
	MATH_ASSERT(step > 0);
	return std::round(value / step) * step;
}


export
long double operator""_deg_to_rad(const long double val){
	return val * deg_to_rad_v<long double>;
}

export
long double operator""_rad_to_deg(const long double val){
	return val * rad_to_deg_v<long double>;
}


export
template <typename T>
[[nodiscard]] MATH_ATTR constexpr T get_normalized(const T& val) noexcept{
	if constexpr(arithmetic<T>){
		return T{val == 0 ? 0 : math::copysign(1, val)};
	} else{
		const auto abs = math::abs(val);
		if(math::zero(abs)) return T{};

		return val * (1 / abs);
	}
}

export
template <typename T, arithmetic V>
[[nodiscard]] MATH_ATTR constexpr T get_normalized(const T& val, const V scl) noexcept{
	if constexpr(arithmetic<T>){
		return T{val == 0 ? 0 : math::copysign(scl, val)};
	} else{
		const auto abs = math::abs(val);
		if(math::zero(abs)) return T{};

		return val * (scl / abs);
	}
}


export
template <typename T>
[[nodiscard]] MATH_ATTR constexpr T get_normalized_nonzero(const T& val) noexcept{
	if constexpr(arithmetic<T>){
		return math::copysign<T>(1, val);
	} else{
		const auto abs = math::abs(val);
		return val * (1 / abs);
	}
}

export
template <typename T, arithmetic V>
[[nodiscard]] MATH_ATTR constexpr T get_normalized_nonzero(const T& val, const V scl) noexcept{
	if constexpr(arithmetic<T>){
		return math::copysign<T>(scl, val);
	} else{
		const auto abs = math::abs(val);
		return val * (scl / abs);
	}
}
}

module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"
#define MATH_ATTR [[nodiscard]] FORCE_INLINE
#define ANGLE_PURE PURE_FN

export module mo_yanxi.math.angle;

import mo_yanxi.math;
import mo_yanxi.tags;
import std;


namespace mo_yanxi::math{

	export
	template <typename T>
	struct backward_forward_ang{
		T forward;
		T backward;
	};

	template <typename T>
	CONST_FN FORCE_INLINE constexpr T mod_(T val, T m) noexcept{
		if consteval{
			while(val >= m){
				val -= m;
			}
			while(val <= -m){
				val += m;
			}
			return val;
		}else{
			return std::fmod(val, m);
		}
	}


	/** @brief Uniformed Angle in [-Pi, Pi) radians*/
	export
	template <std::floating_point T>
	struct uniformed_angle{
		using value_type = T;
	private:
		static constexpr value_type DefMargin = 8 / static_cast<value_type>(std::numeric_limits<std::uint16_t>::max());
		static constexpr value_type Pi = std::numbers::pi_v<value_type>;
		// static constexpr value_type Pi_mod = [](){
		// 	if constexpr (std::same_as<value_type, float>){
		// 		return std::bit_cast<float>(std::bit_cast<std::int32_t>(std::numbers::pi_v<float>) + 1);
		// 	}else{
		// 		return std::bit_cast<double>(std::bit_cast<std::int64_t>(std::numbers::pi_v<double>) + 1);
		// 	}
		// }();
		// static constexpr value_type Bound = std::numbers::pi_v<value_type>;

		static constexpr value_type half_cycles = std::numbers::pi_v<value_type>;
		static constexpr value_type cycles = half_cycles * 2.;
		// static constexpr float HALF_VAL = Bound;


	private:
		value_type ang_{};

		/**
		 * @return Angle in [-pi, pi) radians
		 */
		CONST_FN MATH_ATTR constexpr static value_type getAngleInPi2(value_type rad) noexcept{
			if consteval{
				while(rad >= half_cycles){
					rad -= cycles;
				}
				while(rad < -half_cycles){
					rad += cycles;
				}
				return rad;
			}else{
				return std::remainder(rad, cycles);
			}
		}

		FORCE_INLINE constexpr void clampInternal() noexcept{
			ang_ = uniformed_angle::getAngleInPi2(ang_);
		}

		CONST_FN MATH_ATTR static constexpr value_type simpleClamp(value_type val) noexcept{
			CHECKED_ASSUME(val >= -half_cycles * 3);
			CHECKED_ASSUME(val <= half_cycles * 3);
			return val >= half_cycles ? val - cycles : val < -half_cycles ? val + cycles : val;

		}

		FORCE_INLINE constexpr void simpleClamp() noexcept{
			ang_ = uniformed_angle::simpleClamp(ang_);
		}

	public:
		[[nodiscard]] constexpr uniformed_angle() noexcept = default;

		[[nodiscard]] constexpr uniformed_angle(tags::from_deg_t, const value_type deg) noexcept : ang_(deg * deg_to_rad_v<value_type>){
			clampInternal();
		}

		[[nodiscard]] constexpr explicit(false) uniformed_angle(tags::from_rad_t, const value_type rad) noexcept : ang_(rad){
			clampInternal();
		}

		[[nodiscard]] constexpr explicit(false) uniformed_angle(const value_type rad) noexcept : ang_(uniformed_angle::getAngleInPi2(rad)){

		}

		[[nodiscard]] constexpr uniformed_angle(tags::unchecked_t, const value_type rad) noexcept : ang_(rad){
			assert(ang_ >= -Pi);
			assert(ang_ <= Pi);
		}

		[[nodiscard]] constexpr uniformed_angle(tags::unchecked_t, tags::from_deg_t, const value_type rad) noexcept : ang_(rad * deg_to_rad_v<value_type>){
			assert(ang_ >= -Pi);
			assert(ang_ <= Pi);
		}


		FORCE_INLINE constexpr void clamp(const value_type min, const value_type max) noexcept{
			ang_ = math::clamp(ang_, min, max);
		}

		FORCE_INLINE constexpr void clamp(const value_type maxabs) noexcept{
			assert(maxabs >= 0.f);
			ang_ = math::clamp(ang_, -maxabs, maxabs);
		}

		CONST_FN constexpr friend bool operator==(const uniformed_angle& lhs, const uniformed_angle& rhs) noexcept = default;
		CONST_FN constexpr auto operator<=>(const uniformed_angle&) const noexcept = default;

		ANGLE_PURE FORCE_INLINE constexpr uniformed_angle operator-() const noexcept{
			uniformed_angle rst{*this};
			rst.ang_ = -rst.ang_;
			return rst;
		}

		FORCE_INLINE constexpr uniformed_angle& operator+=(const uniformed_angle other) noexcept{
			ang_ += other.ang_;

			simpleClamp();

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator-=(const uniformed_angle other) noexcept{
			ang_ -= other.ang_;

			simpleClamp();

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator+=(const value_type other) noexcept{
			ang_ += other;

			clampInternal();

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator-=(const value_type other) noexcept{
			ang_ -= other;

			clampInternal();

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator*=(const value_type val) noexcept{
			ang_ *= val;

			if(val < -1 || val > 1){
				clampInternal();
			}

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator/=(const value_type val) noexcept{
			ang_ /= val;

			if(val > -1 || val < 1){
				clampInternal();
			}

			return *this;
		}

		FORCE_INLINE constexpr uniformed_angle& operator%=(const value_type val) noexcept{
			ang_ = math::mod_(ang_, val);

			return *this;
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator+(uniformed_angle lhs, const uniformed_angle rhs) noexcept{
			return lhs += rhs;
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator-(uniformed_angle lhs, const uniformed_angle rhs) noexcept{
			return lhs -= rhs;
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator+(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs += rhs;
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator-(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs -= rhs;
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator+(value_type lhs, const uniformed_angle rhs) noexcept{
			return uniformed_angle{lhs + rhs.ang_};
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator-(value_type lhs, const uniformed_angle rhs) noexcept{
			return uniformed_angle{lhs - rhs.ang_};
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator*(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs *= rhs;
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator/(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs /= rhs;
		}

		ANGLE_PURE FORCE_INLINE constexpr friend uniformed_angle operator%(uniformed_angle lhs, const value_type rhs) noexcept{
			return lhs %= rhs;
		}

		ANGLE_PURE FORCE_INLINE constexpr explicit(false) operator value_type() const noexcept{
			return ang_;
		}

		template <std::floating_point Ty>
		ANGLE_PURE FORCE_INLINE constexpr explicit(false) operator uniformed_angle<Ty>() const noexcept{
			return uniformed_angle{tags::unchecked, ang_};
		}

		ANGLE_PURE MATH_ATTR constexpr value_type radians() const noexcept{
			return ang_;
		}

		ANGLE_PURE MATH_ATTR constexpr value_type degrees() const noexcept{
			return ang_ * rad_to_deg_v<value_type>;
		}

		ANGLE_PURE MATH_ATTR constexpr bool equals(const uniformed_angle other, const value_type margin) const noexcept{
			return math::abs(static_cast<value_type>(*this - other)) < margin;
		}

		ANGLE_PURE MATH_ATTR constexpr bool equals(tags::unchecked_t, const value_type other,
		                                    const value_type margin) const noexcept{
			return math::abs(uniformed_angle::simpleClamp(static_cast<value_type>(*this) - other)) < margin;
		}

		// ANGLE_PURE MATH_ATTR backward_forward_ang<value_type> forward_backward_ang(const uniformed_angle other) const noexcept{
		// 	auto abs = math::abs(ang_ - other.ang_);
		// 	return {abs, cycles - abs};
		// }

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr value_type rotate_toward_nearest_direction(
			const uniformed_angle target
		) const  noexcept{
			const auto dst = target.ang_ - ang_;
			return math::copysign(1, dst >= half_cycles ? -1 : dst < half_cycles ? 1 : dst);
		}

		[[nodiscard]] PURE_FN FORCE_INLINE constexpr value_type rotate_toward_nearest_direction(
			const uniformed_angle target,
			value_type scl
		) const noexcept{
			const auto dst = target.ang_ - ang_;
			return math::copysign(scl, dst >= half_cycles ? -1 : dst < half_cycles ? 1 : dst);
		}

		FORCE_INLINE constexpr void rotate_toward_nearest(
			const uniformed_angle target,
			const value_type delta
		) noexcept{
			const auto dlt = this->rotate_toward_nearest_direction(target, delta);
			ang_ = uniformed_angle::getAngleInPi2(ang_ + dlt);
		}

		FORCE_INLINE constexpr bool rotate_toward_nearest_clamped(
			const uniformed_angle target,
			const value_type delta
		) noexcept{
			assert(delta >= 0);
			const auto dlt = (target - *this).radians();
			bool clp = delta > math::abs(dlt);
			ang_ = uniformed_angle::simpleClamp(ang_ + math::copysign(clp ? dlt : delta, dlt));
			return clp;
		}

		FORCE_INLINE constexpr void slerp(const uniformed_angle target, const value_type progress) noexcept{
			const value_type delta = target - *this;
			ang_ += delta * progress;

			clampInternal();
		}

		[[nodiscard]] ANGLE_PURE FORCE_INLINE constexpr uniformed_angle as_abs(this uniformed_angle self) noexcept{
			self.ang_ = math::abs(self.ang_);
			return self;
		}


		CONST_FN FORCE_INLINE friend value_type distance(const uniformed_angle& lhs, const uniformed_angle& rhs) noexcept{
			return math::abs((lhs - rhs).radians());
		}


		CONST_FN FORCE_INLINE friend value_type abs(const uniformed_angle& v) noexcept{
			return cpo::abs(v.radians());
		}

		CONST_FN FORCE_INLINE friend value_type sqr(const uniformed_angle& v) noexcept{
			return cpo::sqr(v.radians());
		}

		CONST_FN FORCE_INLINE friend uniformed_angle lerp(const uniformed_angle& lhs, const uniformed_angle& rhs, value_type p) noexcept{
			return uniformed_angle{lhs.ang_ + (rhs.ang_ - lhs.ang_) * p};
		}
	};

	export using angle = uniformed_angle<float>;


	export
	uniformed_angle<long double> operator""_deg_to_uang(const long double val){
		return {tags::from_deg, val};
	}

	export
	uniformed_angle<long double> operator""_rad_to_uang(const long double val){
		return {tags::from_rad, val};
	}

}



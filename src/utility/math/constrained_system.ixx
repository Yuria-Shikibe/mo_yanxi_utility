module;

#include "../../../include/mo_yanxi/adapted_attributes.hpp"
#include <cassert>

export module mo_yanxi.math.constrained_system;

export import mo_yanxi.math;
import std;

namespace mo_yanxi::math::constrain_resolve{
	template <typename T, typename V>
	FORCE_INLINE [[nodiscard]] constexpr T get_normalized_(const T& val, const V abs, const V scl) noexcept{
		if constexpr (std::is_arithmetic_v<T>){
			if consteval{
				return T{val == 0 ? 0 : val > 0 ? scl : -scl};
			}else{
				return T{val == 0 ? 0 : std::copysign(scl, val)};
			}
		}else{
			if(math::zero(abs))return T{};

			return val * (scl / abs);
		}
	}

	export
	template <typename T0, typename T1, std::floating_point M>
	[[nodiscard]] CONST_FN constexpr T0 smooth_approach_lerp(
		const T0& approach,
		const T1& initial_first_derivative,
		const M max_second_derivative,
		const M margin,
		const M yaw_correction_factor = M{0.5}
		) noexcept{

		assert(max_second_derivative > 0);
		assert(margin > 0);

		const auto idvLen = math::sqr(initial_first_derivative);
		const auto idvAbs = [&] FORCE_INLINE {
			if constexpr (std::is_arithmetic_v<T1>){
				return math::abs(initial_first_derivative);
			}else{
				return math::sqrt(idvLen);
			}
		}();

		const auto dst = math::abs(approach);
		const auto overhead = idvLen / (2 * max_second_derivative);
		if(math::zero(idvAbs)){
			if(dst <= margin){
				return T0{};
			}
		}else{
			if(dst <= overhead + margin){
				return initial_first_derivative * (-max_second_derivative / idvAbs);
			}
		}


		const auto yaw = constrain_resolve::get_normalized_(initial_first_derivative, idvAbs, -overhead);
		const auto instruction = math::lerp(
			approach,
			math::lerp(approach, yaw, math::map<M, M>(math::dot(approach, yaw), 0, -overhead * dst, 0, 1)), yaw_correction_factor);
		return math::get_normalized(instruction, max_second_derivative);
	}

	export
	template <typename T0, typename T1, std::floating_point M>
	[[nodiscard]] CONST_FN constexpr T0 smooth_approach(
		const T0& approach,
		const T1& initial_first_derivative,
		const M max_second_derivative,
		const M margin = std::numeric_limits<M>::epsilon()
		) noexcept{

		if(max_second_derivative <= 0)return T0{};

		assert(margin >= 0);

		const auto idvLen = math::sqr(initial_first_derivative);
		const auto idvAbs = [&] FORCE_INLINE {
			if constexpr (std::is_arithmetic_v<T1>){
				return math::abs(initial_first_derivative);
			}else{
				return math::sqrt(idvLen);
			}
		}();

		const auto dst = math::abs(approach);
		const auto overhead = idvLen / (2 * max_second_derivative);
		if(math::zero(idvAbs)){
			if(dst <= margin){
				return T0{};
			}
		}else{
			if(dst <= overhead + margin){
				return initial_first_derivative * (-max_second_derivative / idvAbs);
			}
		}

		const auto instruction = approach + constrain_resolve::get_normalized_(initial_first_derivative, idvAbs, -overhead);
		return math::get_normalized(instruction, max_second_derivative);
	}
}
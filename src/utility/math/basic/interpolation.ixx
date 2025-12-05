export module mo_yanxi.math.interpolation;

import std;
import mo_yanxi.math;
import mo_yanxi.concepts;

//OPTM static operator()
//TODO wrap in a struct and expose friend apply operator |
//TODO uses constexpr program to enhance the effiency
namespace mo_yanxi::math::interp::spec{
	template <std::floating_point T, std::floating_point ...ArgT>
	[[nodiscard]] constexpr T cos_n(T progress, ArgT... args) noexcept{
		std::array values{args...};

		T result{0.0};
		for(unsigned i = 0; i < sizeof...(ArgT); ++i){
			T weight = 0.5 * (1.0 - math::cos(2.0 * math::pi * progress / static_cast<T>(sizeof...(ArgT) - 1u)));
			result += values[i] * weight;
		}

		return result;
	}

	template <std::floating_point T, std::size_t size>
	[[nodiscard]] constexpr T cos_n(T progress, std::array<T, size> values) noexcept{
		T result{0.0};
		for(unsigned i = 0; i < values.size(); ++i){
			T weight = 0.5 * (1.0 - math::cos(2.0 * math::pi * progress / static_cast<T>(values.size() - 1u)));
			result += values[i] * weight;
		}

		return result;
	}

	template <auto power>
	struct Pow{
		static constexpr float operator()(float a) noexcept{
			if constexpr(std::floating_point<decltype(power)>){
				if(a <= 0.5f) return std::powf(a * 2, power) * 0.5f;
				return std::powf((a - 1.0f) * 2.0f, power) / (std::fmod(power, 2.0f) == 0.0f ? -2.0f : 2.0f) + 1;

			} else{
				if(a <= 0.5f) return math::pow_integral<power>(a * 2) * 0.5f;
				return math::pow_integral<power>((a - 1.0f) * 2.0f) / ((power % 2 == 0) ? -2.0f : 2.0f) + 1;
			}


		}
	};


	template <auto Func, float powBegin = 0.5f, float minVal = 0.02f>
		requires invocable<decltype(Func), float(float)>
	struct LinePow{
		static constexpr float operator()(float a) noexcept{
			auto t = a < powBegin ? minVal * a / powBegin : (1 - minVal) * Func((a - powBegin) / (1 - powBegin));

			return t;
		}
	};


	template <auto power>
	struct PowIn{
		static constexpr float operator()(float a) noexcept{
			if constexpr(std::floating_point<decltype(power)>){
				return std::powf(a, power);
			} else{
				return math::pow_integral<power>(a);
			}
		}
	};


	template <auto power>
	struct PowOut{
		static constexpr float operator()(float a) noexcept{
			if constexpr(std::floating_point<decltype(power)>){
				return std::powf(a - 1.0f, power) * (std::fmod(power, 2.0f) == 0.0f ? -1.0f : 1.0f) + 1.0f;
			} else{
				return math::pow_integral<power>(a - 1.0f) * (power % 2 == 0 ? -1.0f : 1.0f) + 1.0f;
			}
		}
	};


	struct Exp{
		const float value{}, power{}, min{}, scale{};

		Exp(const float value, const float power) noexcept : value(value),
		                                                     power(power),
		                                                     min{std::powf(value, -power)},
		                                                     //Uses template param when powf is constexpr
		                                                     scale(-1.0f / (1.0f - min)){
		}

		float operator()(const float a) const noexcept{
			if(a <= 0.5f) return (std::powf(value, power * (a * 2 - 1)) - min) * scale / 2;
			return (2 - (std::powf(value, -power * (a * 2 - 1)) - min) * scale) / 2;
		}
	};


	struct ExpIn : Exp{
		[[nodiscard]] ExpIn(const float value, const float power) noexcept
			: Exp(value, power){
		}

		float operator()(const float a) const noexcept{
			return (std::powf(value, power * (a - 1)) - min) * scale;
		}
	};


	struct ExpOut : Exp{
		[[nodiscard]] ExpOut(const float value, const float power) noexcept
			: Exp(value, power){
		}

		float operator()(const float a) const noexcept{
			return 1 - (std::powf(value, -power * a) - min) * scale;
		}
	};


	template <float value, float power, float scale, int bounces>
	struct Elastic{
		static constexpr float RealBounces = bounces * std::numbers::pi_v<float> * (bounces % 2 == 0 ? 1 : -1);

		static float operator()(float a) noexcept{
			if(a <= 0.5f){
				a *= 2;
				return std::powf(value, power * (a - 1)) * sin(a * RealBounces) * scale / 2;
			}
			a = 1 - a;
			a *= 2;
			return 1 - std::powf(value, power * (a - 1)) * sin(a * RealBounces) * scale / 2;
		}
	};


	template <float value, float power, float scale, int bounces>
	struct ElasticIn : Elastic<value, power, scale, bounces>{
		using Elastic<value, power, scale, bounces>::RealBounces;

		static float operator()(float a) noexcept{
			if(a >= 0.99) return 1;
			return std::powf(value, power * (a - 1)) * sin(a * RealBounces) * scale;
		}
	};


	template <float value, float power, float scale, int bounces>
	struct ElasticOut : Elastic<value, power, scale, bounces>{
		using Elastic<value, power, scale, bounces>::RealBounces;

		static float operator()(float a) noexcept{
			if(a == 0) return 0;
			a = 1 - a;
			return 1 - std::powf(value, power * (a - 1)) * sin(a * RealBounces) * scale;
		}
	};


	template <std::size_t bounces>
	struct BounceOut{
		using Bounce = std::array<float, bounces>;
		const Bounce widths{}, heights{};

		[[nodiscard]] constexpr BounceOut(Bounce widths, Bounce heights) noexcept : widths(widths), heights(heights){
		}

		[[nodiscard]] constexpr BounceOut() noexcept{
			static_assert(bounces >= 2 && bounces <= 5, "bounces cannot be < 2 or > 5");

			auto& widths = const_cast<Bounce&>(this->widths);
			auto& heights = const_cast<Bounce&>(this->heights);

			heights[0] = 1;

			switch(bounces){
			case 2 : widths[0] = 0.6f;
				widths[1] = 0.4f;
				heights[1] = 0.33f;
				break;
			case 3 : widths[0] = 0.4f;
				widths[1] = 0.4f;
				widths[2] = 0.2f;
				heights[1] = 0.33f;
				heights[2] = 0.1f;
				break;
			case 4 : widths[0] = 0.34f;
				widths[1] = 0.34f;
				widths[2] = 0.2f;
				widths[3] = 0.15f;
				heights[1] = 0.26f;
				heights[2] = 0.11f;
				heights[3] = 0.03f;
				break;
			case 5 : widths[0] = 0.3f;
				widths[1] = 0.3f;
				widths[2] = 0.2f;
				widths[3] = 0.1f;
				widths[4] = 0.1f;
				heights[1] = 0.45f;
				heights[2] = 0.3f;
				heights[3] = 0.15f;
				heights[4] = 0.06f;
				break;
			default :;
			}

			widths[0] *= 2;
		}

		constexpr float operator()(float a) const noexcept{
			if(a == 1.0f) return 1;
			a += widths[0] / 2;
			float width = 0, height = 0;
			for(int i = 0, n = widths.size(); i < n; i++){
				width = widths[i];
				if(a <= width){
					height = heights[i];
					break;
				}
				a -= width;
			}
			a /= width;
			const float z = 4 / width * height * a;
			return 1 - (z - z * a) * width;
		}
	};


	template <size_t bounces>
	struct Bounce : BounceOut<bounces>{
		[[nodiscard]] constexpr Bounce(const typename BounceOut<bounces>::Bounce& widths,
		                               const typename BounceOut<bounces>::Bounce& heights)
			noexcept : BounceOut<bounces>(widths, heights){
		}

		[[nodiscard]] constexpr Bounce() noexcept = default;

	private:
		using BounceOut<bounces>::widths;

		[[nodiscard]] constexpr float out(float a) const noexcept{
			float test = a + widths[0] / 2;
			if(test < widths[0]) return test / (widths[0] / 2) - 1;
			return BounceOut<bounces>::operator()(a);
		}

	public:
		constexpr float operator()(const float a) const noexcept{
			if(a <= 0.5f) return (1 - out(1 - a * 2)) / 2;
			return out(a * 2 - 1) / 2 + 0.5f;
		}
	};


	template <size_t bounces>
	struct BounceIn : BounceOut<bounces>{
		[[nodiscard]] constexpr BounceIn(const typename BounceOut<bounces>::Bounce& widths,
		                                 const typename BounceOut<bounces>::Bounce& heights)
			noexcept : BounceOut<bounces>(widths, heights){
		}

		[[nodiscard]] constexpr BounceIn() noexcept = default;

		constexpr float operator()(const float a) const noexcept{
			return 1 - BounceOut<bounces>::operator()(1 - a);
		}
	};


	template <float s>
	struct Swing{
		static constexpr float scale = s * 2.0f;

		static constexpr float operator()(float a) noexcept{
			if(a <= 0.5f){
				a *= 2;
				return a * a * ((scale + 1) * a - scale) / 2;
			}
			a -= 1.0f;
			a *= 2;
			return a * a * ((scale + 1) * a + scale) / 2 + 1;
		}
	};

	template <float s>
	struct SwingOut{
		static constexpr float scale = s;

		static constexpr float operator()(float a) noexcept{
			a -= 1.0f;
			return a * a * ((scale + 1) * a + scale) + 1;
		}
	};

	template <float s>
	struct SwingIn{
		static constexpr float scale = s;

		static constexpr float operator()(float a) noexcept{
			return a * a * ((scale + 1) * a - scale);
		}
	};
}

namespace mo_yanxi::math::interp{
	template<typename T>
	concept static_interp = requires(float f){
		T::operator()(f);
	};

	export
	using general_func = std::function<float(float)>;

	export
	using general_func_move_only = std::move_only_function<float(float) const>;

	export
	using no_state_interp_func = std::add_pointer_t<float(float) noexcept>;

	export
	template <std::regular_invocable<float> Fn>
		requires (std::is_nothrow_invocable_r_v<float, Fn, float>)
	struct interp_func{
	private:
		Fn fn;

	public:
		[[nodiscard]] constexpr interp_func() noexcept = default;

		[[nodiscard]] explicit(false) constexpr interp_func(Fn fn) noexcept requires std::is_copy_constructible_v<Fn>
			: fn{fn}{
		}

		constexpr float operator()(const float f) const noexcept requires (!static_interp<Fn>){
			return std::invoke(fn, f);
		}

		static constexpr float operator()(const float f) noexcept requires (static_interp<Fn>){
			return Fn::operator()(f);
		}

		constexpr friend float operator|(const float f, const interp_func& fn) noexcept{
			return fn(f);
		}


		constexpr operator no_state_interp_func() const noexcept requires (static_interp<Fn>){
			return +[] (float f) static constexpr noexcept{
				return Fn::operator()(f);
			};
		}
	};

	export
	template <typename LFn, typename RFn>
	constexpr auto operator|(const interp_func<LFn>& f1, const interp_func<RFn>& f2) noexcept{
		if constexpr(static_interp<LFn> && static_interp<RFn>){
			return interp_func{
					[](float f) static constexpr noexcept{
						return LFn::operator()(RFn::operator()(f));
					}
				};
		} else if constexpr(static_interp<LFn>){
			return interp_func{
					[f2](float f) constexpr noexcept{
						return LFn::operator()(f2(f));
					}
				};
		} else if constexpr(static_interp<RFn>){
			return interp_func{
					[f1](float f) constexpr noexcept{
						return f1(RFn::operator()(f));
					}
				};
		} else{
			return interp_func{
					[f1, f2](float v) constexpr noexcept{
						return f1(f2(v));
					}
				};
		}
	}


	export{
		template <float margin_in, float margin_out = 1.f>
		inline constexpr auto linear_margined = interp_func{[](const float x) static noexcept{
			return math::fma(x, margin_out - margin_in, margin_in);
		}};

		template <float margin_in, float margin_out = 1.f>
		inline constexpr auto linear_map = interp_func{[](const float x) static noexcept{
			return curve(x, margin_in, margin_out);
		}};

		inline constexpr interp_func linear = [](const float x) static noexcept{
			return x;
		};

		inline constexpr interp_func reverse = [](const float x) static noexcept{
			return 1.0f - x;
		};

		inline constexpr interp_func smooth = [](const float x) static noexcept{
			return x * x * (3.0f - 2.0f * x);
		};

		inline constexpr interp_func smooth2 = [](float a) static noexcept{
			a = a * a * (3.0f - 2.0f * a);
			return a * a * (3.0f - 2.0f * a);
		};

		inline constexpr interp_func one = [](float) static noexcept{
			return 1.0f;
		};

		inline constexpr interp_func zero = [](float) static noexcept{
			return 0.0f;
		};

		inline constexpr interp_func slope = [](const float a) static noexcept{
			return 1.0f - std::abs(a - 0.5f) * 2.0f;
		};

		inline constexpr interp_func smoother = [](const float a) static noexcept{
			return a * a * a * (a * (a * 6.0f - 15.0f) + 10.0f);
		};

		inline constexpr auto& fade = smoother;

		inline constexpr interp_func burst = spec::LinePow<math::sqr, 0.925f>{};

		inline constexpr interp_func pow2 = spec::Pow<2.f>{};
		/** Slow, then fast. */
		inline constexpr interp_func pow2In = spec::PowIn<2>{};
		inline constexpr auto& slowFast = pow2In;
		/** Fast, then slow. */
		inline constexpr interp_func pow2Out = spec::PowOut<2>{};
		inline constexpr auto& fastSlow = pow2Out;
		inline constexpr interp_func pow2InInverse = [](const float a) static noexcept{
			return std::sqrtf(a);
		};

		inline constexpr interp_func pow2OutInverse = [](const float a) static noexcept{
			return std::sqrtf(-a + 1.0f);
		};

		inline constexpr interp_func pow3 = spec::Pow<3>{};
		inline constexpr interp_func pow3In = spec::PowIn<3>{};
		inline constexpr interp_func pow3Out = spec::PowOut<3>{};
		inline constexpr interp_func pow3InInverse = [](const float a) static noexcept{
			return std::cbrtf(a);
		};
		inline constexpr interp_func pow3OutInverse = [](const float a) static noexcept{
			return std::cbrtf(1.0f - a);
		};
		inline constexpr interp_func pow4 = spec::Pow<4>{};
		inline constexpr interp_func pow4In = spec::PowIn<4>{};
		inline constexpr interp_func pow4Out = spec::PowOut<4>{};
		inline constexpr interp_func pow5 = spec::Pow<5>{};
		inline constexpr interp_func pow5In = spec::PowIn<5>{};
		inline constexpr interp_func pow10In = spec::PowIn<10>{};
		inline constexpr interp_func pow10Out = spec::PowOut<10>{};
		inline constexpr interp_func pow5Out = spec::PowOut<5>{};
		inline constexpr interp_func sine = [](const float a) static constexpr noexcept{
			return (1.0f - math::cos(a * math::pi)) * 0.5f;
		};
		inline constexpr interp_func sineIn = [](const float a) static constexpr noexcept{
			return 1.0f - math::cos(a * math::pi * 0.5f);
		};
		inline constexpr interp_func sineOut = [](const float a) static constexpr noexcept{
			return math::sin(a * math::pi * 0.5f);
		};

		// inline const interp_func exp10 = spec::Exp(2, 10);
		// inline const interp_func exp10In = spec::ExpIn(2, 10);
		// inline const interp_func exp10Out = spec::ExpOut(2, 10);
		// inline const interp_func exp5 = spec::Exp(2, 5);
		// inline const interp_func exp5In = spec::ExpIn(2, 5);
		// inline const interp_func exp5Out = spec::ExpOut(2, 5);

		inline constexpr interp_func circle = [](float a) static noexcept{
			if(a <= 0.5f){
				a *= 2;
				return (1 - std::sqrtf(1 - a * a)) / 2;
			}
			a -= 1.0f;
			a *= 2;
			return (std::sqrtf(1 - a * a) + 1) / 2;
		};
		inline constexpr interp_func circleIn = [](const float a) static noexcept{
			return 1 - std::sqrtf(1 - a * a);
		};
		inline constexpr interp_func circleOut = [](float a) static noexcept{
			a -= 1.0f;
			return std::sqrtf(1 - a * a);
		};

		inline constexpr interp_func elastic = spec::Elastic<2.f, 10.f, 7.f, 1>{};
		inline constexpr interp_func elasticIn = spec::ElasticIn<2.f, 10.f, 6.f, 1>{};
		inline constexpr interp_func elasticOut = spec::ElasticOut<2.f, 10.f, 7.f, 1>{};
		inline constexpr interp_func swing = spec::Swing<1.5f>();
		inline constexpr interp_func swingIn = spec::SwingIn<2.0f>();
		inline constexpr interp_func swingOut = spec::SwingOut<2.0f>();
		// inline constexpr interp_func bounce = spec::Bounce<4>();
		// inline constexpr interp_func bounceIn = spec::BounceIn<4>();
		// inline constexpr interp_func bounceOut = spec::BounceOut<4>();
	}

	template <typename Rng>
	using SizeTy = decltype(std::ranges::size(std::declval<Rng&>()));


	template <typename Rng>
	using AccessTy = decltype(std::declval<Rng&>()[SizeTy<Rng>{}]);

	export
	template <
		typename Rng,
		typename SzTy = SizeTy<Rng>,
		std::invocable<AccessTy<Rng>, AccessTy<Rng>, float> Lerp
	>
		requires requires(const Rng& rng, SzTy sz){
			{ rng[sz] };
		}
	constexpr std::remove_cvref_t<AccessTy<Rng>> rangeLerp(Rng&& range, const float progress, Lerp lerpFn) noexcept{
		SzTy sz = std::ranges::size(range);
		if(sz == 0){
			return std::remove_cvref_t<AccessTy<Rng>>{};
		}
		const auto boundF = static_cast<float>(sz - 1);

		const float pos = progress * boundF;

		const SzTy prev = static_cast<SzTy>(math::clamp<float>(progress * boundF, 0, boundF));
		const SzTy next = prev + 1;

		if(next >= sz){
			return range[prev];
		}

		const float mod = pos - static_cast<float>(prev);
		return std::invoke(lerpFn, range[prev], range[next], mod);
	}

	export
	template <
		typename Rng,
		typename SzTy = SizeTy<Rng>,
		std::invocable<AccessTy<Rng>, AccessTy<Rng>, float> Lerp
	>
		requires requires(const Rng& rng, SzTy sz){
			{ rng[sz] };
		}
	constexpr std::remove_cvref_t<AccessTy<Rng>> rangeLerp_reversed(Rng&& range, const float progress,
	                                                                Lerp lerpFn) noexcept{
		SzTy sz = std::ranges::size(range);
		if(sz == 0){
			return std::remove_cvref_t<AccessTy<Rng>>{};
		}
		const auto boundF = static_cast<float>(sz - 1);

		const float pos = (1 - progress) * boundF;

		const SzTy prev = static_cast<SzTy>(math::clamp<float>(progress * boundF, 0, boundF));
		const SzTy next = prev + 1;

		if(next >= sz){
			return range[prev];
		}

		const float mod = static_cast<float>(next) - pos;
		return std::invoke(lerpFn, range[next], range[prev], mod);
	}


	export
	struct trivial_interp{
		no_state_interp_func func{};

		[[nodiscard]] constexpr trivial_interp() = default;

		[[nodiscard]] constexpr explicit(false) trivial_interp(no_state_interp_func func)
			: func(func){
		}

		[[nodiscard]] constexpr explicit(false) trivial_interp(const std::convertible_to<no_state_interp_func> auto& func)
			: func(func){
		}

		constexpr float operator()(float prog) const noexcept{
			if(!func)return prog;
			return func(prog);
		}

		explicit constexpr operator bool() const noexcept{
			return func != nullptr;
		}
	};

	// constexpr float f = rangeLerp_reversed(std::vector{0.f, 1.f}, 0.5f, [](float a, float b, float p){
	// 	return math::lerp(a, b, p);
	// });
}

// export namespace math::DiffApproach{
// 	// ret(src, tgt, delta)
// 	using DiffApproachFunc = float(float, float, float);
// 	using DiffApproachFuncPtr = std::decay_t<DiffApproachFunc>;
//
// 	// std::function<>
//
// 	DiffApproachFuncPtr ratioApproaching = math::lerp;
// 	DiffApproachFuncPtr linearApproaching = math::approach;
// 	DiffApproachFuncPtr instantApproaching = [](float, const float tgt, float) noexcept{
// 		return tgt;
// 	};
// }

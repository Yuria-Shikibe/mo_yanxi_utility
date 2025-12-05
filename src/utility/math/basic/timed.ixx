//
// Created by Matrix on 2024/3/8.
//

export module mo_yanxi.math.timed;

export import mo_yanxi.math;
// export import Math.Interpolation;
import mo_yanxi.concepts;
import std;

//TODO is this namespace appropriate?
export namespace mo_yanxi::math{
	struct timed {
		float lifetime;
		float time;

		template <bool autoClamp = false>
		constexpr void set(const float lifetime, const float time = .0f){
			if constexpr (autoClamp){
				this->lifetime = max(lifetime, 0.0f);
				this->time = clamp(time, 0.0f, lifetime);
			}else{
				this->lifetime = lifetime;
				this->time = time;
			}

		}

		explicit constexpr operator bool() const noexcept{
			return time >= lifetime;
		}


		/**
		 * @param delta advance time delta
		 * @return if time reached lifetime
		 */
		constexpr bool update_and_get(const float delta) noexcept{
			time += delta;
			if(time >= lifetime){
				time = lifetime;
				return true;
			}

			return false;
		}

		template <bool autoClamp = false>
		constexpr void update(const float delta) noexcept{
			time += delta;

			if constexpr (autoClamp){
				time = clamp(time, 0.0f, lifetime);
			}
		}

		template <bool autoClamp = false>
		[[nodiscard]] constexpr float get() const noexcept{
			if constexpr (autoClamp){
				return clamp(time / lifetime);
			}

			return time / lifetime;
		}

		template <bool autoClamp = true>
		[[nodiscard]] constexpr float get_with(const float otherLifetime) const noexcept{
			if constexpr (autoClamp){
				return clamp(time / otherLifetime);
			}

			return time / otherLifetime;
		}

		template <invocable<float(float)> Func>
		[[nodiscard]] constexpr float get(Func interp) const noexcept(std::is_nothrow_invocable_r_v<float, Func, float>){
			return std::invoke(interp, get());
		}

		/**
		 * @brief [margin, 1]
		 */
		[[nodiscard]] constexpr float get_margin(const float margin_at_min) const noexcept{
			return margin_at_min + get() * (1 - margin_at_min);
		}

		[[nodiscard]] constexpr float get_inv() const noexcept{
			return 1 - get();
		}

		[[nodiscard]] float get_slope() const noexcept{
			return slope(get());
		}
	};
}

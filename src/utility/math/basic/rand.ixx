module;

#include <cassert>

export module mo_yanxi.math.rand;

import std;

export namespace mo_yanxi::math {
	/**
	 * TODO adapt to std::random_generator ?
	 *
	 * @brief Impl <b> XorShift </b> Random
	 *
	 * @author Inferno
	 * @author davebaol
	 */
	class rand {
	public:
		using seed_t = std::uint64_t;

	private:
		/** Normalization constant for IEEE754 double. */
		static constexpr double NORM_DOUBLE = 1.0 / static_cast<double>(1ll << 53);

		/** Normalization constant for IEEE754 float. */
		static constexpr float NORM_FLOAT = 1.0f / static_cast<float>(1ll << 24);

		static constexpr seed_t murmurHash3(seed_t x) noexcept {
			x ^= x >> 33;
			x *= 0xff51afd7ed558ccdull;
			x ^= x >> 33;
			x *= 0xc4ceb9fe1a85ec53ull;
			x ^= x >> 33;
			return x;
		}

		/** The first half of the internal state of this pseudo-random number generator. */
		seed_t seed0{};

		/** The second half of the internal state of this pseudo-random number generator. */
		seed_t seed1{};

	public:
		static auto get_default_seed() noexcept {
			// 优化：使用纳秒级高精度时钟，防止同秒内生成相同种子
			return static_cast<seed_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		}

		rand() noexcept {
			set_seed(get_default_seed());
		}

		/**
		 * @param seed the initial seed
		 */
		constexpr explicit rand(const seed_t seed) noexcept {
			set_seed(seed);
		}

		/**
		 * @param seed0 the first part of the initial seed
		 * @param seed1 the second part of the initial seed
		 */
		constexpr rand(const seed_t seed0, const seed_t seed1) noexcept {
			set_state(seed0, seed1);
		}

		constexpr std::uint64_t next() noexcept {
			seed_t s1       = this->seed0;
			const seed_t s0 = this->seed1;
			this->seed0     = s0;
			s1 ^= s1 << 23;
			return (this->seed1 = s1 ^ s0 ^ s1 >> 17ull ^ s0 >> 26ull) + s0;
		}

		constexpr std::uint64_t operator ()() noexcept{
			return next();
		}

		// 优化：利用 Concept 拆分整型逻辑
		template <std::integral T>
		constexpr T next() noexcept {
			if constexpr (std::same_as<T, bool>) {
				return (next() & 1) != 0;
			} else {
				return static_cast<T>(next());
			}
		}

		// 优化：利用 Concept 拆分浮点逻辑
		template <std::floating_point T>
		constexpr T next() noexcept {
			if constexpr (std::same_as<T, float>) {
				return static_cast<float>(next() >> 40) * NORM_FLOAT;
			} else if constexpr (std::same_as<T, double>) {
				return static_cast<double>(next() >> 11) * NORM_DOUBLE;
			} else {
				static_assert(std::same_as<T, void>, "unsupported floating type");
			}
		}

		/**
		 * Returns the next pseudo-random, uniformly distributed {int} value from this random number generator's sequence.
		 */
		constexpr int next_int() noexcept {
			return static_cast<int>(next());
		}

		/**
		 * @param n_exclusive the positive bound on the random number to be returned.
		 * @return the next pseudo-random {int} value between {0} (inclusive) and {n} (exclusive).
		 */
		constexpr int next_int(const int n_exclusive) noexcept {
			return static_cast<int>(next(static_cast<std::uint64_t>(n_exclusive)));
		}

		/**
		 * Returns a pseudo-random, uniformly distributed {long} value between 0 (inclusive) and the specified value (exclusive).
		 * @param n_exclusive the positive bound on the random number to be returned.
		 * @return [0, n)
		 */
		constexpr std::uint64_t next(const std::uint64_t n_exclusive) noexcept {
			assert(n_exclusive != 0);
			if (n_exclusive == 1) return 0;

			// 优化：使用位掩码代替缓慢的取模运算
			const std::uint64_t mask = std::bit_ceil(n_exclusive) - 1;
			std::uint64_t value;
			do {
				value = next() & mask;
			} while (value >= n_exclusive);

			return value;
		}

		/**
		 * The given seed is passed twice through a hash function.
		 * @param _seed a nonzero seed for this generator.
		 */
		constexpr void set_seed(const seed_t _seed) noexcept {
			const seed_t seed0_val = murmurHash3(_seed == 0 ? std::numeric_limits<seed_t>::lowest() : _seed);
			set_state(seed0_val, murmurHash3(seed0_val));
		}

		template <std::floating_point T>
		constexpr bool chance(const T chance_val) noexcept {
			return next<T>() < chance_val;
		}

		template <std::floating_point T>
		constexpr T inaccuracy_scale(const T inaccuracy) noexcept {
			return static_cast<T>(1) + this->range(inaccuracy);
		}

		template <std::integral T>
		constexpr T range(const T rng) noexcept {
			return static_cast<T>(this->next(static_cast<std::uint64_t>(rng) * 2 + 1)) - rng;
		}

		template <std::floating_point T>
		constexpr T range(const T rng) noexcept {
			return next<T>() * rng * 2 - rng;
		}

		template <std::integral T>
		constexpr T random(const T max_inclusive) noexcept {
			return static_cast<T>(this->next(static_cast<std::uint64_t>(max_inclusive) + 1));
		}

		template <std::floating_point T>
		constexpr T random(const T max_inclusive) noexcept {
			return next<T>() * max_inclusive;
		}

		template <typename T1, typename T2>
			requires (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>)
		constexpr auto random(const T1 min, const T2 max) noexcept -> std::common_type_t<T1, T2> {
			using T = std::common_type_t<T1, T2>;
			if constexpr (std::integral<T>) {
				assert(min <= max);
				return min + static_cast<T>(this->next(static_cast<std::uint64_t>(max - min) + 1));
			} else {
				return min + (max - min) * next<T>();
			}
		}

		template <typename T1, typename T2>
			requires (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>)
		constexpr auto operator()(const T1 min, const T2 max) noexcept -> std::common_type_t<T1, T2> {
			return this->random(min, max);
		}

		template <std::integral T>
		constexpr T operator()(const T rng) noexcept {
			return this->range(rng);
		}

		template <std::floating_point T>
		constexpr T operator()(const T rng) noexcept {
			return this->range(rng);
		}

		template <typename T = float>
			requires (std::is_floating_point_v<T>)
		constexpr T random_direction_deg() noexcept {
			return this->random<T>(static_cast<T>(360));
		}

		/**
		 * Sets the internal state of this generator.
		 * @param s0 the first part of the internal state
		 * @param s1 the second part of the internal state
		 */
		constexpr void set_state(const seed_t s0, const seed_t s1) noexcept {
			this->seed0 = s0;
			this->seed1 = s1;
		}
	};
}
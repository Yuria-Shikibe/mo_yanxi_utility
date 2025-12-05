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

		// constexpr int next(const int bits) noexcept {
		// 	return static_cast<int>(next() & (1ull << bits) - 1ull);
		// }

	public:
		static auto get_default_seed() noexcept {
			return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		}

		rand() noexcept { // NOLINT(*-use-equals-default)
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
			this->seed0       = s0;
			s1 ^= s1 << 23;
			return (this->seed1 = s1 ^ s0 ^ s1 >> 17ull ^ s0 >> 26ull) + s0;
		}

		constexpr std::uint64_t operator ()() noexcept{
			return next();
		}

		template <typename T>
			requires (std::is_arithmetic_v<T>)
		constexpr T next() noexcept{
			if constexpr (std::integral<T>){
				return static_cast<T>(next());
			}else if constexpr (std::floating_point<T>){
				if constexpr (std::same_as<T, float>){
					return static_cast<float>(next() >> 40) * NORM_FLOAT; // NOLINT(*-narrowing-conversions)
				}else if constexpr (std::same_as<T, double>){
					return static_cast<double>(next() >> 11) * NORM_DOUBLE;
				}else{
					static_assert(false, "unsupported floating type");
				}
			}else if constexpr (std::same_as<T, bool>){
				return (next() & 1) != 0;
			}else{
				static_assert(false, "unsupported arithmetic type");
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
			return next(n_exclusive);
		}

		/**
		 * Returns a pseudo-random, uniformly distributed {long} value between 0 (inclusive) and the specified value (exclusive),
		 * drawn from this random number generator's sequence. The algorithm used to generate the value guarantees that the result is
		 * uniform, provided that the sequence of 64-bit values produced by this generator is.
		 * @param n_exclusive the positive bound on the random number to be returned.
		 * @return (0, n]
		 */
		constexpr std::uint64_t next(const std::uint64_t n_exclusive) noexcept {
			assert(n_exclusive != 0);
			for(;;) {
				const std::uint64_t bits  = next() >> 1ull;
				const std::uint64_t value = bits % n_exclusive;
				if(bits >= value + (n_exclusive - 1)) return value;
			}
		}

		/**
		 * The given seed is passed twice through a hash function. This way, if the user passes a small value we avoid the short
		 * irregular transient associated with states having a very small number of bits set.
		 * @param _seed a nonzero seed for this generator (if zero, the generator will be seeded with @link std::numeric_limits<SeedType>::lowest() @endlink ).
		 */
		constexpr void set_seed(const seed_t _seed) noexcept {
			const seed_t seed0 = murmurHash3(_seed == 0 ? std::numeric_limits<seed_t>::lowest() : _seed);
			set_state(seed0, murmurHash3(seed0));
		}

		template <std::floating_point T>
		constexpr bool chance(const T chance) noexcept {
			return next<T>() < chance;
		}

		template <std::floating_point T>
		constexpr T inaccuracy_scale(const T inaccuracy) noexcept {
			return static_cast<T>(1) + this->range(inaccuracy);
		}

		template <typename T>
			requires (std::is_arithmetic_v<T>)
		constexpr T range(const T range) noexcept {
			if constexpr (std::integral<T>){
				//TODO support unsigned to signed??
				return this->next_int(range * 2 + 1) - range;
			}else if constexpr (std::floating_point<T>){
				return next<T>() * range * 2 - range;
			}else{
				static_assert(false, "unsupported arithmetic type");
			}

		}

		template <typename T>
			requires (std::is_arithmetic_v<T>)
		constexpr T random(const T max_inclusive) noexcept {
			if constexpr (std::integral<T>){
				//TODO support unsigned to signed??
				return this->next_int(max_inclusive + 1);
			}else if constexpr (std::floating_point<T>){
				return next<T>() * max_inclusive;
			}else{
				static_assert(false, "unsupported arithmetic type");
			}
		}

		template <typename T1, typename T2>
			requires (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>)
		constexpr auto random(const T1 min, const T2 max) noexcept -> std::common_type_t<T1, T2> {
			using T = std::common_type_t<T1, T2>;

			if constexpr (std::integral<T>){
				assert(min <= max);
				return min + this->next_int(max - min + 1);
			}else{
				return min + (max - min) * next<T>();
			}
		}

		template <typename T1, typename T2>
			requires (std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>)
		constexpr auto operator()(const T1 min, const T2 max) noexcept -> std::common_type_t<T1, T2>{
			return this->random(min, max);
		}

		template <typename T>
			requires (std::is_arithmetic_v<T>)
		constexpr T operator()(const T range) noexcept{
			return this->range(range);
		}

		template <typename T = float>
			requires (std::is_arithmetic_v<T>)
		constexpr T random_direction_deg() noexcept{
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

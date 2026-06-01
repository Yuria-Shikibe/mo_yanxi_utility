module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.heterogeneous.open_addr_hash;

export import mo_yanxi.heterogeneous;

import std;

namespace mo_yanxi{

	struct type_index_equal_to__not_null{
		using is_transparent = void;

		FORCE_INLINE static bool operator()(std::nullptr_t, const std::type_index b) noexcept {
			return b == typeid(std::nullptr_t);
		}

		FORCE_INLINE static bool operator()(const std::type_index b, std::nullptr_t) noexcept {
			return b == typeid(std::nullptr_t);
		}

		FORCE_INLINE static bool operator()(const std::type_index a, const std::type_index b) noexcept {
			return a == b;
		}

		FORCE_INLINE static constexpr bool operator()(std::nullptr_t, std::nullptr_t) noexcept {
			return true;
		}
	};

	struct type_index_hasher{
		using is_transparent = void;
		static constexpr std::hash<std::type_index> hasher{};

		FORCE_INLINE static std::size_t operator()(const std::type_index val) noexcept {
			return hasher(val);
		}

		FORCE_INLINE static std::size_t operator()(std::nullptr_t) noexcept {
			return hasher(typeid(std::nullptr_t));
		}
	};

	export
	template <typename T>
	using type_unordered_map = std::unordered_map<
			std::type_index,
			T,
			type_index_hasher,
			type_index_equal_to__not_null>;

	export
	template <typename T>
	using type_fixed_hash_map = type_unordered_map<T>;

	export
	template <typename V>
	using string_open_addr_hash_map = string_hash_map<V>;
}

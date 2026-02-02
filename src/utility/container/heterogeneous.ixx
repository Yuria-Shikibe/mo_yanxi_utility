module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.heterogeneous;

import std;
import mo_yanxi.meta_programming;

export namespace mo_yanxi::transparent{
struct string_equal_to{
	using is_transparent = void;

	// 通用比较：只要两个操作数都能转换为 string_view
	template <typename T, typename U>
		requires std::convertible_to<T, std::string_view> && std::convertible_to<U, std::string_view>
	FORCE_INLINE static constexpr bool operator()(const T& lhs, const U& rhs) noexcept{
		return std::string_view(lhs) == std::string_view(rhs);
	}
};

template <template <typename> typename Comp>
	requires std::regular_invocable<Comp<std::string_view>, std::string_view, std::string_view>
struct string_comparator_of{
	static constexpr Comp<std::string_view> comp{};
	using is_transparent = void;

	template <typename T, typename U>
		requires std::convertible_to<T, std::string_view> && std::convertible_to<U, std::string_view>
	FORCE_INLINE static auto operator()(const T& a, const U& b) noexcept{
		return comp(std::string_view(a), std::string_view(b));
	}
};

struct string_hasher{
	using is_transparent = void;
	static constexpr std::hash<std::string_view> hasher{};

	template <typename T>
		requires std::convertible_to<T, std::string_view>
	FORCE_INLINE static std::size_t operator()(const T& val) noexcept{
		return hasher(std::string_view(val));
	}
};

template <typename T>
struct ptr_equal_to{
	using base_type = T;
	using is_transparent = void;
	constexpr bool operator()(const base_type* a, const base_type* b) const noexcept{ return a == b; }

	template <typename PtrTy1, typename PtrTy2>
		requires requires{
			requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy1&>())), base_type*>;
			requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy2&>())), base_type*>;
		}
	static constexpr bool operator()(const PtrTy1& a, const PtrTy2& b) noexcept{
		return std::to_address(a) == std::to_address(b);
	}

	template <typename PtrTy>
		requires requires{
			requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy&>())), base_type*>;
		}
	static constexpr bool operator()(const base_type* a, const PtrTy& b) noexcept{ return a == std::to_address(b); }

	template <typename PtrTy>
		requires requires{
			requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy&>())), base_type*>;
		}
	static constexpr bool operator()(const PtrTy& b, const base_type* a) noexcept{ return a == std::to_address(b); }
};

template <typename T>
struct ptr_hasher{
	using base_type = T;
	using is_transparent = void;
	static constexpr std::hash<base_type*> hasher{};
	constexpr std::size_t operator()(const base_type* a) const noexcept{ return hasher(a); }

	template <typename PtrTy>
		requires(std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy&>())), base_type*>)
	static constexpr std::size_t operator()(const PtrTy& a) noexcept{
		return hasher(std::to_address(a));
	}
};
}

namespace mo_yanxi{
template <typename T, auto T::* ptr>
	requires(mo_yanxi::default_hashable<typename mptr_info<decltype(ptr)>::value_type>)
struct projection_hash{
	using type = typename mptr_info<decltype(ptr)>::value_type;
	using is_transparent = void;
	static constexpr std::hash<type> hasher{};

	static constexpr std::size_t operator()(const type& val) noexcept{ return hasher(val); }

	static constexpr std::size_t operator()(const T& val) noexcept{ return hasher(std::invoke(ptr, val)); }
};

template <typename T, typename V, V T::* ptr>
	requires(mo_yanxi::default_hashable<T> && mo_yanxi::default_hashable<typename mptr_info<decltype(ptr)>::value_type>)
struct [[deprecated]] projection_equal_to{
	using type = typename mptr_info<decltype(ptr)>::value_type;
	using is_transparent = void;
	static constexpr std::equal_to<type> equal{};

	static constexpr bool operator()(const T& a, const T& b) noexcept{
		return equal(std::invoke(ptr, a), std::invoke(ptr, b));
	}

	static constexpr bool operator()(const type& a, const T& b) noexcept{ return equal(a, std::invoke(ptr, b)); }
};

export template <typename Alloc = std::allocator<std::string>>
struct string_hash_set : std::unordered_set<std::string, transparent::string_hasher, transparent::string_equal_to,
		Alloc>{
private:
	using self_type = std::unordered_set<std::string, transparent::string_hasher, transparent::string_equal_to, Alloc>;

public:
	using self_type::unordered_set;
	using self_type::insert;

	// 扩充 insert 支持通用类型
	template <typename K>
		requires std::convertible_to<K, std::string_view>
	decltype(auto) insert(K&& key){
		// 注意：如果 key 是 string_view，这里需要构建 std::string 才能插入
		// 这里为了保持原有逻辑语义，如果 Heterogeneous lookup 发现存在则不插入，否则构造 string
		if(auto it = this->find(std::string_view(key)); it != this->end()){
			return std::pair{it, false};
		}
		return this->emplace(std::string(std::forward<K>(key)));
	}
};

export
template <typename Key, typename V, typename Alloc = std::allocator<std::pair<const Key, V>>>
class basic_string_hash_map : public
std::unordered_map<Key, V, transparent::string_hasher, transparent::string_equal_to, Alloc>{
private:
	using self_type = std::unordered_map<Key, V, transparent::string_hasher, transparent::string_equal_to, Alloc>;

public:
	using std::unordered_map<Key, V, transparent::string_hasher, transparent::string_equal_to, Alloc>::unordered_map;

	// 通用的 at
	template <typename S, typename K>
		requires std::convertible_to<K, std::string_view>
	auto&& at(this S&& self, const K& key){
		if(auto itr = self.find(key); itr != self.end()){
			return std::forward_like<S>(itr->second);
		} else{
			throw std::out_of_range("key not found");
		}
	}

	// 通用的 try_find
	template <typename K>
		requires std::convertible_to<K, std::string_view>
	V* try_find(const K& key){
		if(const auto itr = this->find(key); itr != this->end()){
			return &itr->second;
		}
		return nullptr;
	}

	template <typename K>
		requires std::convertible_to<K, std::string_view>
	const V* try_find(const K& key) const{
		if(const auto itr = this->find(key); itr != this->end()){
			return &itr->second;
		}
		return nullptr;
	}

	using self_type::insert_or_assign;
	using self_type::try_emplace;

	// 通用的 try_emplace
	// 如果 K 支持转为 string_view (用于查找) 且 Key 支持从 K 构造 (用于插入)
	template <typename K, class... Arg>
		requires (std::convertible_to<K, std::string_view> && std::constructible_from<Key, K&&>)
	std::pair<typename self_type::iterator, bool> try_emplace(K&& key, Arg&&... val){
		// 先尝试异构查找
		std::string_view sv = key;
		if(auto itr = this->find(sv); itr != this->end()){
			return {itr, false};
		} else{
			// 未找到，构造 Key 并插入
			return this->self_type::try_emplace(Key(std::forward<K>(key)), std::forward<Arg>(val)...);
		}
	}

	// 通用的 insert_or_assign
	template <typename K, class... Arg>
		requires (std::convertible_to<K, std::string_view> && std::constructible_from<Key, K&&>)
	std::pair<typename self_type::iterator, bool> insert_or_assign(K&& key, Arg&&... val){
		// insert_or_assign 标准行为通常不直接支持异构 key 用于 slot 查找优化，
		// 除非 C++20/23 扩展，这里我们手动转发为 Key 类型以确保安全
		return this->self_type::insert_or_assign(Key(std::forward<K>(key)), std::forward<Arg>(val)...);
	}

	using self_type::operator[];

	// 通用的 operator[]
	template <typename K>
		requires (std::convertible_to<K, std::string_view> && std::constructible_from<Key, K&&>)
	V& operator[](K&& key){
		std::string_view sv = key;
		if(auto itr = this->find(sv); itr != this->end()){
			return itr->second;
		}
		// 使用 Key(key) 构造并插入，利用了 Key 支持从 K (或 string_view) 构造的特性
		return this->self_type::emplace(Key(std::forward<K>(key)), typename self_type::mapped_type{}).first->second;
	}
};

// 保持原有 string_hash_map 的名称和行为
export template <typename V, typename Alloc = std::allocator<std::pair<const std::string, V>>>
using string_hash_map = basic_string_hash_map<std::string, V, Alloc>;
}

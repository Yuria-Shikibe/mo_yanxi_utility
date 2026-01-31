module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.heterogeneous;

import std;
import mo_yanxi.meta_programming;

export namespace mo_yanxi::transparent{
	struct string_equal_to{
		using is_transparent = void;

		FORCE_INLINE
		constexpr bool operator()(const std::string_view a, const std::string_view b) const noexcept {
			return a == b;
		}

		FORCE_INLINE
		constexpr bool operator()(const std::string_view a, const std::string& b) const noexcept {
			return a == static_cast<std::string_view>(b);
		}

		FORCE_INLINE
		constexpr bool operator()(const std::string& b, const std::string_view a) const noexcept {
			return static_cast<std::string_view>(b) == a;
		}

		FORCE_INLINE
		constexpr bool operator()(const std::string& b, const std::string& a) const noexcept {
			return b == a;
		}
	};

	template <template <typename > typename Comp>
		requires std::regular_invocable<Comp<std::string_view>, std::string_view, std::string_view>
	struct string_comparator_of{
		static constexpr Comp<std::string_view> comp{};
		using is_transparent = void;
		auto operator()(const std::string_view a, const std::string_view b) const noexcept {
			return comp(a, b);
		}

		auto operator()(const std::string_view a, const std::string& b) const noexcept {
			return comp(a, b);
		}

		auto operator()(const std::string& a, const std::string_view b) const noexcept {
			return comp(a, b);
		}

		auto operator()(const std::string& a, const std::string& b) const noexcept {
			return comp(a, b);
		}
	};

	struct string_hasher{
		using is_transparent = void;
		static constexpr std::hash<std::string_view> hasher{};

		std::size_t operator()(const std::string_view val) const noexcept {
			return hasher(val);
		}

		std::size_t operator()(const std::string& val) const noexcept {
			return hasher(val);
		}
	};

	template <typename T>
	struct ptr_equal_to{
		using base_type = T;
		using is_transparent = void;

		constexpr bool operator()(const base_type* a, const base_type* b) const noexcept {
			return a == b;
		}

		template <typename PtrTy1, typename PtrTy2>
			requires requires{
				requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy1&>())), base_type*>;
				requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy2&>())), base_type*>;
			}
		constexpr bool operator()(const PtrTy1& a, const PtrTy2& b) const noexcept{
			return std::to_address(a) == std::to_address(b);
		}

		template <typename PtrTy>
			requires requires{
				requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy&>())), base_type*>;
			}
		constexpr bool operator()(const base_type* a, const PtrTy& b) const noexcept{
			return a == std::to_address(b);
		}

		template <typename PtrTy>
			requires requires{
				requires std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy&>())), base_type*>;
			}
		constexpr bool operator()(const PtrTy& b, const base_type* a) const noexcept{
			return a == std::to_address(b);
		}
	};

	template <typename T>
	struct ptr_hasher{
		using base_type = T;
		using is_transparent = void;
		static constexpr std::hash<base_type*> hasher{};

		constexpr std::size_t operator()(const base_type* a) const noexcept {
			return hasher(a);
		}

		template <typename PtrTy>
			requires (std::equality_comparable_with<decltype(std::to_address(std::declval<PtrTy&>())), base_type*>)
		constexpr std::size_t operator()(const PtrTy& a) const noexcept {
			return hasher(std::to_address(a));
		}
	};
}

namespace mo_yanxi{

	template <typename T, auto T::* ptr>
		requires (mo_yanxi::default_hashable<typename mptr_info<decltype(ptr)>::value_type>)
	struct projection_hash{
		using type = typename mptr_info<decltype(ptr)>::value_type;
		using is_transparent = void;

		static constexpr std::hash<type> hasher{};

		constexpr std::size_t operator()(const type& val) const noexcept {
			return hasher(val);
		}

		constexpr std::size_t operator()(const T& val) const noexcept {
			return hasher(std::invoke(ptr, val));
		}
	};

	template <typename T, typename V, V T::* ptr>
		requires (mo_yanxi::default_hashable<T> && mo_yanxi::default_hashable<typename mptr_info<decltype(ptr)>::value_type>)
	struct [[deprecated]] projection_equal_to{
		using type = typename mptr_info<decltype(ptr)>::value_type;
		using is_transparent = void;
		static constexpr std::equal_to<type> equal{};

		constexpr bool operator()(const T& a, const T& b) const noexcept{
			return equal(std::invoke(ptr, a), std::invoke(ptr, b));
		}

		constexpr bool operator()(const type& a, const T& b) const noexcept{
			return equal(a, std::invoke(ptr, b));
		}
	};

	export
	template <typename Alloc = std::allocator<std::string>>
	struct string_hash_set : std::unordered_set<std::string, transparent::string_hasher, transparent::string_equal_to, Alloc>{
	private:
		using self_type = std::unordered_set<std::string, transparent::string_hasher, transparent::string_equal_to, Alloc>;

	public:
		using self_type::unordered_set;
		using self_type::insert;

		decltype(auto) insert(const std::string_view string){
			return this->insert(std::string(string));
		}

		decltype(auto) insert(const char* string){
			return this->insert(std::string(string));
		}
	};

// using V = int;
// using Alloc = std::allocator<std::pair<const std::string, V>>;

	export
	template <typename V, typename Alloc = std::allocator<std::pair<const std::string, V>>>
	class string_hash_map : public std::unordered_map<std::string, V, transparent::string_hasher, transparent::string_equal_to, Alloc>{
	private:
		using self_type = std::unordered_map<std::string, V, transparent::string_hasher, transparent::string_equal_to, Alloc>;

	public:
		using std::unordered_map<std::string, V, transparent::string_hasher, transparent::string_equal_to, Alloc>::unordered_map;

		auto& at(this auto& self, const std::string_view key) {
			if(auto itr = self.find(key); itr != self.end()){
				return itr->second;
			}else{
				throw std::out_of_range("key not found");
			}
		}

		V at(const std::string_view key, const V& def) const requires std::is_copy_assignable_v<V>{
			if(const auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}
			return def;
		}

		V at(const std::string_view key, V&& def) const requires std::is_copy_assignable_v<V>{
			if(const auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}
			return std::move(def);
		}

		V* try_find(const std::string_view key){
			if(const auto itr = this->find(key); itr != this->end()){
				return &itr->second;
			}
			return nullptr;
		}

		const V* try_find(const std::string_view key) const {
			if(const auto itr = this->find(key); itr != this->end()){
				return &itr->second;
			}
			return nullptr;
		}

		using self_type::insert_or_assign;
		using self_type::try_emplace;

		template <class ...Arg>
		std::pair<typename self_type::iterator, bool> try_emplace(const std::string_view key, Arg&& ...val){
			if(auto itr = this->find(key); itr != this->end()){
				return {itr, false};
			}else{
				return this->self_type::try_emplace(std::string(key), std::forward<Arg>(val) ...);
			}
		}

		template <class ...Arg>
		std::pair<typename self_type::iterator, bool> try_emplace(const char* key, Arg&& ...val){
			return this->try_emplace(std::string_view{key}, std::forward<Arg>(val) ...);
		}

		template <class ...Arg>
		std::pair<typename self_type::iterator, bool> insert_or_assign(const std::string_view key, Arg&& ...val) {
			return this->self_type::insert_or_assign(static_cast<std::string>(key), std::forward<Arg>(val) ...);
		}

		template <std::size_t sz, class ...Arg>
		std::pair<typename self_type::iterator, bool> insert_or_assign(const char (&key)[sz], Arg&& ...val) {
			return this->insert_or_assign(std::string_view(key, sz), std::forward<Arg>(val) ...);
		}

		using self_type::operator[];

		V& operator[](const std::string_view key) {
			if(auto itr = this->find(key); itr != this->end()){
				return itr->second;
			}

			return this->self_type::emplace(std::string(key), typename self_type::mapped_type{}).first->second;
		}

		V& operator[](const char* key) {
			return operator[](std::string_view(key));
		}
	};
}
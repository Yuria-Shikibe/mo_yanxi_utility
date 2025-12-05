// © 2017-2020 Erik Rigtorp <erik@rigtorp.se>
// SPDX-License-Identifier: MIT

/*
HashMap

A high performance hash map. Uses open addressing with linear
probing.

Advantages:
  - Predictable performance. Doesn't use the allocator unless load factor
    grows beyond 50%. Linear probing ensures cash efficency.
  - Deletes items by rearranging items and marking slots as empty instead of
    marking items as deleted. This is keeps performance high when there
    is a high rate of churn (many paired inserts and deletes) since otherwise
    most slots would be marked deleted and probing would end up scanning
    most of the table.

Disadvantages:
  - Significant performance degradation at high load factors.
  - Maximum load factor hard coded to 50%, memory inefficient.
  - Memory is not reclaimed on erase.
 */

module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.open_addr_hash_map;

import std;

namespace mo_yanxi{
	export
	struct bad_hash_map_key : std::runtime_error{
		[[nodiscard]] bad_hash_map_key() : std::runtime_error{"bad_hash_map_key"} {}

		using std::runtime_error::runtime_error;
	};
	//TODO using uninitialized memory and placement instead of assignment
	//TODO provide bucket impl selection?
	//TODO is assert good?

	template <typename T>
	concept Transparent = requires{
		typename T::is_transparent;
	};

	template <typename K, typename V>
	struct kv_entry{
		K key;
		V val;

		using first_type = K;
		using second_type = V;

		template <std::size_t I>
		constexpr decltype(auto) get() noexcept{
			if constexpr (I == 0){
				return key;
			}else if constexpr (I == 1){
				return val;
			}else{
				static_assert(false, "subscript out of bound");
			}
		}

		template <std::size_t I>
		constexpr decltype(auto) get() const noexcept{
			if constexpr (I == 0){
				return key;
			}else if constexpr (I == 1){
				return val;
			}else{
				static_assert(false, "subscript out of bound");
			}
		}
	};
}

/*
export namespace std{
	template <std::size_t I, typename K, typename V>
	constexpr decltype(auto) get(ext::kv_entry<K, V>& p) noexcept{
		if constexpr(I == 0){
			return p.key;
		} else if constexpr(I == 1){
			return p.val;
		} else{
			static_assert(false, "subscript out of bound");
		}
	}

	template <std::size_t I, typename K, typename V>
	constexpr decltype(auto) get(const ext::kv_entry<K, V>& p) noexcept{
		if constexpr(I == 0){
			return p.key;
		} else if constexpr(I == 1){
			return p.val;
		} else{
			static_assert(false, "subscript out of bound");
		}
	}

	template <std::size_t I, typename K, typename V>
	constexpr decltype(auto) get(ext::kv_entry<K, V>&& p) noexcept{
		if constexpr(I == 0){
			return p.key;
		} else if constexpr(I == 1){
			return p.val;
		} else{
			static_assert(false, "subscript out of bound");
		}
	}

	template <std::size_t I, typename K, typename V>
	constexpr decltype(auto) get(const ext::kv_entry<K, V>&& p) noexcept{
		if constexpr(I == 0){
			return p.key;
		} else if constexpr(I == 1){
			return p.val;
		} else{
			static_assert(false, "subscript out of bound");
		}
	}
}
*/

template <typename K, typename V>
struct std::tuple_size<mo_yanxi::kv_entry<K, V>> : std::integral_constant<std::size_t, 2>{};

template <typename K, typename V>
struct std::tuple_element<0, mo_yanxi::kv_entry<K, V>> : std::type_identity<K>{};

template <typename K, typename V>
struct std::tuple_element<1, mo_yanxi::kv_entry<K, V>> : std::type_identity<V>{};

namespace mo_yanxi{
	//TODO remove the nonsense reinterpret cast and other UB

	struct pointer_transparent_hasher{
		template <typename T>
			requires (std::is_pointer_v<T>)
		static constexpr std::size_t operator()(T ptr) noexcept{
			static constexpr std::hash<const void*> hash{};
			return hash(ptr);
		}

		using is_transparent = void;
	};

	export
	template <
		typename Key, typename Val, typename EmptyKey, auto emptyKey,
		typename Hash = std::hash<Key>,
		typename KeyEqual = std::equal_to<Key>,
		std::invocable<const EmptyKey&> Convertor = std::identity,
		typename Allocator = std::allocator<std::pair<Key, Val>>> /*Capture*/
	struct fixed_open_addr_hash_map{
		using key_type = Key;
		using mapped_type = Val;
		using hasher = Hash;
		using key_equal = KeyEqual;

	private:
		template <typename K>
			static constexpr bool isHasherValid = (std::same_as<K, key_type> || Transparent<hasher>) && std::is_invocable_r_v<
				std::size_t, hasher, const K&>;

		template <typename K>
		static constexpr bool isEqualerValid = std::same_as<K, key_type> || Transparent<key_equal>;

		static constexpr decltype(auto) getStaticEmptyKey() noexcept{
			return empty_key();
		}

	public:
		using value_type = std::pair<const key_type, mapped_type>;


	private:
		template <typename K>
		FORCE_INLINE
		constexpr static bool is_empty_key(const K& key) noexcept{
			if constexpr(std::predicate<key_equal, const K&> && requires{
				typename key_equal::is_direct;
			}){
				return KeyEqualer(key);
			}else{
				return KeyEqualer(getStaticEmptyKey(), key);
			}
		}

		struct mapped_buffer{ // NOLINT(*-pro-type-member-init)
			static constexpr std::size_t mapped_type_size = sizeof(mapped_type);
			static constexpr std::size_t mapped_type_align = alignof(mapped_type);
			alignas(mapped_type_align) std::array<std::byte, mapped_type_size> buffer;

			[[nodiscard]] constexpr const mapped_type* data() const noexcept{
				return reinterpret_cast<const mapped_type*>(buffer.data());
			}

			[[nodiscard]] constexpr mapped_type* data() noexcept{
				return reinterpret_cast<mapped_type*>(buffer.data());
			}
		};

		struct kv_storage{
			key_type key{};
			ADAPTED_NO_UNIQUE_ADDRESS mapped_buffer value;

			[[nodiscard]] constexpr explicit(false) kv_storage(const key_type key)
				: key(key){
			}

			template <typename T>
				requires std::constructible_from<key_type, T>
			[[nodiscard]] constexpr explicit(false) kv_storage(T&& key)
				: key(std::forward<T>(key)){
			}

			[[nodiscard]] constexpr kv_storage() : key(getStaticEmptyKey()){}

			constexpr explicit operator bool() const noexcept{
				return !fixed_open_addr_hash_map::is_empty_key(key);
			}

			constexpr ~kv_storage(){
				try_destroy();
			}

			constexpr void try_destroy() noexcept{
				if constexpr (std::is_trivially_destructible_v<mapped_type>)return;
				if(!fixed_open_addr_hash_map::is_empty_key(key)){
					std::destroy_at(value.data());
				}
			}

			constexpr void destroy() noexcept{
				if constexpr (std::is_trivially_destructible_v<mapped_type>)return;

				std::destroy_at(value.data());
			}

			template <typename ...Args>
				requires (std::constructible_from<mapped_type, Args...>)
			constexpr void emplace(Args&& ...args) noexcept(std::is_nothrow_constructible_v<mapped_type, Args...>){
				std::construct_at(value.data(), std::forward<Args>(args)...);
			}

			constexpr void move_assign(mapped_type&& arg) noexcept(std::is_nothrow_move_assignable_v<mapped_type>){
				*value.data() = std::move(arg);
			}

			kv_storage(const kv_storage& other)
				: key{other.key}{
				if(*this){
					if constexpr (std::is_trivially_copy_constructible_v<mapped_type>){
						value = other.value;
					}else{
						this->emplace(*other.value.data());
					}
				}
			}

			kv_storage(kv_storage&& other) noexcept
				: key{std::exchange(other.key, getStaticEmptyKey())}{
				if(*this){
					if constexpr (std::is_trivially_move_constructible_v<mapped_type>){
						value = other.value;
					}else{
						this->emplace(std::move(*other.value.data()));
						other.destroy();
					}
				}
			}

			kv_storage& operator=(const kv_storage& other){
				if(this == &other) return *this;
				try_destroy();
				key = other.key;
				if(*this){
					if constexpr (std::is_trivially_copy_constructible_v<mapped_type>){
						value = other.value;
					}else{
						this->emplace(*other.value.data());
					}
				}
				return *this;
			}

			kv_storage& operator=(kv_storage&& other) noexcept{
				if(this == &other) return *this;
				try_destroy();
				key = std::exchange(other.key, getStaticEmptyKey());
				if(*this){
					if constexpr (std::is_trivially_move_constructible_v<mapped_type>){
						value = other.value;
					}else{
						this->emplace(std::move(*other.value.data()));
						other.destroy();
					}
				}
				return *this;
			}
		};

	public:
		using size_type = std::size_t;
		using value_type_internal = kv_storage;

		using allocator_type = std::allocator<kv_storage>/*Allocator*/;
		using reference = value_type&;
		using const_reference = const value_type&;

		using buckets = std::vector<value_type_internal, allocator_type>;

	private:
		static constexpr hasher Hasher{};
		static constexpr key_equal KeyEqualer{};

		template <bool addConst = false>
		struct hm_iterator_static{
			using container_type = std::conditional_t<addConst, const fixed_open_addr_hash_map, fixed_open_addr_hash_map>;
			using difference_type = std::ptrdiff_t;
			using base_iterator = decltype(std::ranges::begin(std::declval<container_type&>().buckets_));
			using iterator_category = std::forward_iterator_tag;
			using value_type = std::conditional_t<addConst, const value_type, value_type>;

			[[nodiscard]] hm_iterator_static() = default;

			constexpr bool operator==(const hm_iterator_static& other) const noexcept = default;

			constexpr hm_iterator_static& operator++(){
				++current;
				advance_past_empty();
				return *this;
			}

			constexpr hm_iterator_static operator++(int){
				auto itr = *this;
				++(*this);
				return itr;
			}

			/*constexpr*/
			auto operator*() const noexcept{
				if constexpr(addConst){
					return std::pair<const key_type&, const mapped_type&>{current->key, *current->value.data()};
				} else{
					return std::pair<const key_type&, mapped_type&>{current->key, *current->value.data()};
				}
			}

			/*constexpr*/
			auto* operator->() const noexcept{
				//TODO UB HERE

				assert(current != sentinel);
				if constexpr(addConst){
					return std::launder(reinterpret_cast<const std::pair<const key_type, mapped_type>*>(std::to_address(current)));
				} else{
					return std::launder(reinterpret_cast<std::pair<const key_type, mapped_type>*>(std::to_address(current)));
				}
			}

			constexpr explicit hm_iterator_static(container_type* hm) noexcept :
				current(std::ranges::begin(hm->buckets_)),
				sentinel(std::ranges::end(hm->buckets_)){
				advance_past_empty();
			}

			constexpr hm_iterator_static(container_type* hm, const size_type idx) noexcept :
				current(std::ranges::begin(hm->buckets_) + idx), sentinel(std::ranges::end(hm->buckets_)){}

		private:
			template <bool oAddConst>
			explicit(false) hm_iterator_static(const hm_iterator_static<oAddConst>& other)
				: current(other.current), sentinel(other.sentinel){}

			constexpr void advance_past_empty() noexcept{
				while(current != sentinel && fixed_open_addr_hash_map::is_empty_key(current->key)){
					++current;
				}
			}

			[[nodiscard]] constexpr kv_storage& raw() const noexcept{
				return *current;
			}

			[[nodiscard]] constexpr std::size_t getIdx(const base_iterator begin) const noexcept{
				return std::distance(begin, current);
			}

			base_iterator current{};
			base_iterator sentinel{};

			friend container_type;
		};

	public:
		using iterator = hm_iterator_static<false>;
		using const_iterator = hm_iterator_static<true>;

	public:
		explicit constexpr fixed_open_addr_hash_map(
			const size_type bucket_count = 16,
			const allocator_type& alloc = allocator_type{})
		: buckets_(alloc){
			if constexpr (std::is_copy_constructible_v<mapped_type>){
				buckets_.resize(std::bit_ceil(bucket_count), key_type{empty_key()});
			}else{
				buckets_.reserve(std::bit_ceil(bucket_count));
				for(size_type i = 0; i < bucket_count; ++i){
					buckets_.emplace_back(empty_key());
				}
			}

		}

		constexpr fixed_open_addr_hash_map(const fixed_open_addr_hash_map& other, const size_type bucket_count)
			: fixed_open_addr_hash_map(bucket_count, other.get_allocator()){
			for(auto it = other.begin(); it != other.end(); ++it){
				this->transfer_raw(it.raw());
			}
		}

		constexpr fixed_open_addr_hash_map(fixed_open_addr_hash_map&& other, const size_type bucket_count)
			: fixed_open_addr_hash_map(bucket_count, other.get_allocator()){
			for(auto it = other.begin(); it != other.end(); ++it){
				this->transfer_raw(std::move(it.raw()));
			}
		}

		[[nodiscard]] constexpr allocator_type get_allocator() const noexcept{
			return buckets_.get_allocator();
		}

		// Iterators
		[[nodiscard]] constexpr iterator begin() noexcept{ return iterator(this); }

		[[nodiscard]] constexpr const_iterator begin() const noexcept{ return const_iterator{this}; }

		[[nodiscard]] constexpr const_iterator cbegin() const noexcept{ return const_iterator{this}; }

		[[nodiscard]] constexpr iterator end() noexcept{ return iterator(this, buckets_.size()); }

		[[nodiscard]] constexpr const_iterator end() const noexcept{
			return const_iterator(this, buckets_.size());
		}

		[[nodiscard]] constexpr const_iterator cend() const noexcept{
			return const_iterator(this, buckets_.size());
		}

		// Capacity
		[[nodiscard]] constexpr bool empty() const noexcept{ return size() == 0; }

		[[nodiscard]] constexpr size_type size() const noexcept{ return size_; }

		[[nodiscard]] constexpr size_type max_size() const noexcept{ return buckets_.max_size() / 2; }

		// Modifiers
		constexpr void clear() noexcept{
			for(auto& b : buckets_){
				if(!fixed_open_addr_hash_map::is_empty_key(b.key)){
					b.key = empty_key();
					if constexpr (!std::is_trivially_destructible_v<std::remove_cv_t<mapped_type>>){
						b.destroy();
					}

				}
			}
			size_ = 0;
		}

		constexpr void seemly_clear() noexcept requires (std::is_trivially_destructible_v<mapped_type> && std::is_copy_assignable_v<key_type>){
			for(auto& b : buckets_){
				if(!fixed_open_addr_hash_map::is_empty_key(b.key)){
					b.key = empty_key();
				}
			}
			size_ = 0;
		}

		constexpr std::pair<iterator, bool> insert(const value_type& value){
			return this->emplace_impl(value.first, value.second);
		}

		constexpr std::pair<iterator, bool> insert(value_type&& value){
			return this->emplace_impl(value.first, std::move(value.second));
		}

		template <typename... Args>
		constexpr std::pair<iterator, bool> emplace(Args&&... args){
			return this->emplace_impl(std::forward<Args>(args)...);
		}

		constexpr void erase(const iterator it){ this->erase_impl(it); }

		constexpr size_type erase(const key_type& key){ return this->erase_impl(key); }

		template <typename K>
		constexpr size_type erase(const K& x){ return this->erase_impl(x); }

		// Lookup
		constexpr mapped_type& at(const key_type& key){ return this->at_impl(key); }

		template <typename K>
		constexpr mapped_type& at(const K& x){ return this->at_impl(x); }

		[[nodiscard]] constexpr const mapped_type& at(const key_type& key) const{
			return this->at_impl(key);
		}

		template <typename K>
		[[nodiscard]] constexpr const mapped_type& at(const K& x) const{
			return this->at_impl(x);
		}

		constexpr mapped_type& operator[](const key_type& key) requires (std::is_default_constructible_v<mapped_type>){
			return this->emplace_impl(key).first->second;
		}

		[[nodiscard]] constexpr size_type count(const key_type& key) const noexcept{
			return this->count_impl(key);
		}

		template <typename K>
		[[nodiscard]] constexpr bool contains(const K& key) const noexcept{
			return this->find_impl(key) != end();
		}

		template <typename... Args>
			requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
		constexpr std::pair<iterator, bool> insert_or_assign(key_type&& key, Args&&... args){
			if(iterator itr = this->find_impl(key); itr != end()){
				itr->second = mapped_type{std::forward<Args>(args)...};
				return std::pair{itr, false};
			} else{
				return this->template emplace_impl<true>(std::move(key), std::forward<Args>(args)...);
			}
		}

		template <typename... Args>
			requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
		constexpr std::pair<iterator, bool> insert_or_assign(const key_type& key, Args&&... args){
			if(iterator itr = this->find_impl(key); itr != end()){
				itr->second = mapped_type{std::forward<Args>(args)...};
				return std::pair{itr, false};
			} else{
				return this->template emplace_impl<true>(key, std::forward<Args>(args)...);
			}
		}

		template <typename... Args>
			requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
		constexpr std::pair<iterator, bool> try_emplace(key_type&& key, Args&&... args){
			if(auto itr = this->find_impl(key); itr != end()){
				return std::pair{itr, false};
			}

			return this->template emplace_impl<true>(std::move(key), std::forward<Args>(args)...);
		}

		template <typename... Args>
			requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
		constexpr std::pair<iterator, bool> try_emplace(const key_type& key, Args&&... args){
			if(auto itr = this->find_impl(key); itr != end()){
				return std::pair{itr, false};
			}

			return this->template emplace_impl<true>(key, std::forward<Args>(args)...);
		}

		template <typename K>
		[[nodiscard]] constexpr size_type count(const K& x) const noexcept{
			return this->count_impl(x);
		}

		[[nodiscard]] constexpr iterator find(const key_type& key) noexcept{ return this->find_impl(key); }

		template <typename K>
		[[nodiscard]] constexpr iterator find(const K& x) noexcept{ return this->find_impl(x); }

		[[nodiscard]] constexpr const_iterator find(const key_type& key) const noexcept{ return this->find_impl(key); }

		template <typename K>
		[[nodiscard]] constexpr const_iterator find(const K& x) const noexcept{
			return this->find_impl(x);
		}

		// Bucket interface
		[[nodiscard]] constexpr size_type bucket_count() const noexcept{ return buckets_.size(); }

		[[nodiscard]] constexpr size_type max_bucket_count() const noexcept{ return buckets_.max_size(); }

		// Hash policy
		constexpr void rehash(size_type count){
			count = std::max(count, size() * 2);
			fixed_open_addr_hash_map other(std::move(*this), count);
			std::swap(*this, other);
		}

		constexpr void reserve(const size_type count){
			if(count * 2 > buckets_.size()){
				rehash(count * 2);
			}
		}

		constexpr void reserve_exact(const size_type count){
			if(count > buckets_.size()){
				rehash(count);
			}
		}

		// Observers
		[[nodiscard]] constexpr hasher hash_function() const noexcept{
			return Hasher;
		}

		[[nodiscard]] constexpr key_equal key_eq() const noexcept{
			return KeyEqualer;
		}

		[[nodiscard]] static constexpr decltype(auto) empty_key() noexcept{
			return std::invoke(key_convertor, fixed_empty_key);
		}

	protected:
		template <bool assert_no_equal = false, std::convertible_to<key_type> K, typename... Args>
			requires (std::constructible_from<mapped_type, Args...> && std::is_move_assignable_v<mapped_type>)
		constexpr std::pair<iterator, bool> emplace_impl(K&& key, Args&&... args){
			if(fixed_open_addr_hash_map::is_empty_key(key)){
				throw bad_hash_map_key{};
			}

			//TODO here
			reserve(size_ + 1);
			for(std::size_t idx = this->key_to_idx(key);; idx = probe_next(idx)){
				auto& kv = buckets_[idx];
				if(fixed_open_addr_hash_map::is_empty_key(kv.key)){
					kv.emplace(std::forward<Args>(args)...);
					kv.key = std::forward<K>(key);
					size_++;
					return {iterator(this, idx), true};
				}

				if constexpr (!assert_no_equal){
					if(KeyEqualer(kv.key, key)){
						return {iterator(this, idx), false};
					}
				}

			}
		}

		constexpr void transfer_raw(const kv_storage& insert_kv){
			assert(!fixed_open_addr_hash_map::is_empty_key(insert_kv.key) && "empty key shouldn't be used");

			reserve(size_ + 1);
			for(std::size_t idx = this->key_to_idx(insert_kv.key);; idx = probe_next(idx)){
				auto& kv = buckets_[idx];
				if(fixed_open_addr_hash_map::is_empty_key(kv.key)){
					kv = insert_kv;
					size_++;
					return;
				} else if(KeyEqualer(kv.key, insert_kv.key)){
					return;
				}

			}
		}

		constexpr void transfer_raw(kv_storage&& insert_kv){
			assert(!fixed_open_addr_hash_map::is_empty_key(insert_kv.key) && "empty key shouldn't be used");

			reserve(size_ + 1);
			for(std::size_t idx = this->key_to_idx(insert_kv.key);; idx = probe_next(idx)){
				auto& kv = buckets_[idx];
				if(fixed_open_addr_hash_map::is_empty_key(kv.key)){
					kv = std::move(insert_kv);
					size_++;
					return;
				} else if(KeyEqualer(kv.key, insert_kv.key)){
					return;
				}
			}
		}

		constexpr void erase_impl(const iterator it) noexcept{
			std::size_t bucket = it.getIdx(buckets_.begin());
			for(std::size_t idx = probe_next(bucket);; idx = probe_next(idx)){
				auto& kv = buckets_[idx];
				if(fixed_open_addr_hash_map::is_empty_key(kv.key)){
					buckets_[bucket].key = empty_key();
					buckets_[bucket].destroy();
					size_--;
					return;
				}

				const std::size_t ideal = this->key_to_idx(kv.key);
				if(diff(bucket, ideal) < diff(idx, ideal)){
					// swap, bucket is closer to ideal than idx
					buckets_[bucket] = std::move(kv);
					bucket = idx;
				}
			}
		}

		template <typename K>
		constexpr size_type erase_impl(const K& key) noexcept{
			auto it = this->find_impl(key);
			if(it != end()){
				this->erase_impl(it);
				return 1;
			}
			return 0;
		}

		template <typename K>
		constexpr mapped_type& at_impl(const K& key){
			const auto it = this->find_impl(key);
			if(it != end()){
				return it->second;
			}
			throw std::out_of_range("HashMap::at");
		}

		template <typename K>
		constexpr const mapped_type& at_impl(const K& key) const{
			return const_cast<fixed_open_addr_hash_map*>(this)->at_impl(key);
		}

		template <typename K>
		constexpr std::size_t count_impl(const K& key) const noexcept{
			return this->find_impl(key) == end() ? 0 : 1;
		}

		template <typename K>
		constexpr iterator find_impl(const K& key) noexcept{
			static_assert(isEqualerValid<K>, "invalid equaler");

			if(fixed_open_addr_hash_map::is_empty_key(key))return end();

			for(std::size_t idx = this->key_to_idx(key);; idx = probe_next(idx)){
				const auto& tKey = buckets_[idx].key;
				if(KeyEqualer(tKey, key)){
					return iterator(this, idx);
				}

				if(fixed_open_addr_hash_map::is_empty_key(tKey)){
					return end();
				}
			}
		}

		template <typename K>
		constexpr const_iterator find_impl(const K& key) const noexcept{
			return const_cast<fixed_open_addr_hash_map*>(this)->find_impl(key);
		}

		template <typename K>
		constexpr std::size_t key_to_idx(const K& key) const noexcept(noexcept(Hasher(key))){
			static_assert(isHasherValid<K>, "invalid hasher");
			const std::size_t mask = buckets_.size() - 1;
			return Hasher(key) % mask;
		}

		[[nodiscard]] constexpr std::size_t probe_next(const std::size_t idx) const noexcept{
			const std::size_t mask = buckets_.size() - 1;
			return (idx + 1) % mask;
		}

		[[nodiscard]] constexpr std::size_t diff(const std::size_t a, const std::size_t b) const noexcept{
			const std::size_t mask = buckets_.size() - 1;
			const std::size_t dst = a > b ? a - b : b - a;
			return (buckets_.size() + dst) % mask;
		}

	private:
		buckets buckets_{};
		std::size_t size_{};

		static constexpr Convertor key_convertor{};
		static constexpr EmptyKey fixed_empty_key{[]() constexpr {
			if constexpr (std::same_as<std::in_place_t, decltype(emptyKey)>){
				return EmptyKey();
			}else{
				return emptyKey;
			}
		}()};
	};

	export
	template <
		typename Key, typename Val, auto emptyKey = std::in_place,
		typename Hash = std::hash<Key>,
		typename KeyEqual = std::equal_to<Key>,
		std::invocable<const decltype(emptyKey)&> Convertor = std::identity,
		typename Allocator = std::allocator<std::pair<Key, Val>>>
	using fixed_open_hash_map = fixed_open_addr_hash_map<
		Key, Val, std::conditional_t<std::same_as<decltype(emptyKey), std::in_place_t>, Key, decltype(emptyKey)>, emptyKey, Hash, KeyEqual, Convertor, Allocator>;

	export
	template <
		typename Key, typename Val,
		typename Hash = pointer_transparent_hasher,
		typename KeyEqual = std::equal_to<void>,
		typename Allocator = std::allocator<std::pair<Key, Val>>>
	// requires std::is_pointer_v<Key>
	using pointer_hash_map = fixed_open_addr_hash_map<
		Key, Val, std::nullptr_t, nullptr, Hash, KeyEqual, std::identity, Allocator>;


	// using M = fixed_open_addr_hash_map<int, float, int, 0>;
	// static_assert(std::ranges::range<M>);
	// static_assert(std::indirectly_readable<M::iterator>);
	// static_assert(std::input_iterator<M::iterator>);
	// static_assert(std::ranges::input_range<M>);

	/*export
	template <
		typename Key, typename Val,
		typename Hash = std::hash<Key>,
		typename KeyEqual = std::equal_to<Key>,
		typename Allocator = std::allocator<std::pair<Key, Val>>>
	class open_address_hash_map : public open_addr_hash_map_base<
		open_address_hash_map<Key, Val, Hash, KeyEqual, Allocator>, Key, Val, Hash, KeyEqual, Allocator>{
		using base = open_addr_hash_map_base<open_address_hash_map, Key, Val, Hash, KeyEqual, Allocator>;

	public:
		using base::open_addr_hash_map_base;

		[[nodiscard]] constexpr auto& empty_key() const noexcept{
			return empty_key_;
		}

	private:
		typename base::key_type empty_key_{};
	};

	export
	template <
		typename Key, typename Val,
		typename EmptyKey, EmptyKey emptyKey,
		typename Hash = std::hash<Key>,
		typename KeyEqual = std::equal_to<Key>,
		std::invocable<const EmptyKey&> Convertor = std::identity,
		typename Allocator = std::allocator<std::pair<Key, Val>>>
		requires requires{
			requires std::assignable_from<Key&, std::invoke_result_t<const Convertor&, const EmptyKey&>>;
			requires std::predicate<KeyEqual, const Key&, const EmptyKey&>;
			requires std::predicate<KeyEqual, const EmptyKey&, const Key&>;
		}
	class fixed_open_address_hash_map :
		public open_addr_hash_map_base<
			fixed_open_address_hash_map<Key, Val, EmptyKey, emptyKey, Hash, KeyEqual, Convertor, Allocator>,
			Key, Val, Hash, KeyEqual, Allocator>
	{
		using base = open_addr_hash_map_base<fixed_open_address_hash_map, Key, Val, Hash, KeyEqual, Allocator>;

	public:
		static constexpr EmptyKey fixed_empty_key = emptyKey;

		explicit constexpr fixed_open_address_hash_map(
			const typename base::size_type bucket_count = 16,
			const typename base::allocator_type& alloc = typename base::allocator_type{})
			: base(empty_key(), bucket_count, alloc){

		}

		constexpr fixed_open_address_hash_map(const fixed_open_address_hash_map& other, const typename base::size_type bucket_count)
			: base(other, bucket_count){
		}

	private:
		friend base;

		[[nodiscard]] constexpr decltype(auto) empty_key() const noexcept{
			return std::invoke(convertor_, fixed_empty_key);
		}


		ADAPTED_NO_UNIQUE_ADDRESS
		Convertor convertor_{};
	};
	*/
}
/*
v1 (0, 0 )
v2 (0, 12) -> v2(3, 11)
v3 (0, 10) -> v3(4, 6 )
v4 (0, 2 )
v5 (2, 18)
v6 (3, 25) -> v6(5, 20)
v7 (4, 24) -> v7(5, 22)


*/
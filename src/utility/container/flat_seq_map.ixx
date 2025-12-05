export module mo_yanxi.flat_seq_map;

import std;

namespace mo_yanxi{
	template <typename K, typename V, typename = std::less<K>>
	struct flat_seq_span;

	export
	template <typename K, typename V>
	struct flat_seq_map_storage{
		using key_type = K;
		using mapped_type = V;

		key_type key;
		mapped_type value;
	};

	export
	template <typename K, typename V, typename Comp = std::less<K>, typename Cont = std::vector<flat_seq_map_storage<K, V>>>
	struct flat_seq_map{
		using key_type = K;
		using mapped_type = V;
		using comp = Comp;

	private:
		template <typename, typename, typename>
		friend struct flat_seq_span;


	public:
		using value_type = flat_seq_map_storage<key_type, mapped_type>;
		static_assert(std::same_as<std::ranges::range_value_t<Cont>, value_type>);

	private:
		Cont values{};

		static constexpr decltype(auto) try_to_underlying(const key_type& key) noexcept{
			if constexpr(std::is_enum_v<key_type>){
				return std::to_underlying(key);
			} else{
				return key;
			}
		}

		template <typename Key>
		static constexpr decltype(auto) try_to_underlying(Key&& key) noexcept{
			return std::forward<Key>(key);
		}

		[[nodiscard]] static constexpr decltype(auto) get_key_proj(const value_type& value) noexcept{
			return flat_seq_map::try_to_underlying(value.key);
		}

	public:
		template <typename S>
		constexpr auto get_values(this S&& self) noexcept{
			return std::forward_like<S>(self.values) | std::views::transform(&value_type::value);
		}

		template <typename Key, std::ranges::input_range Rng = std::initializer_list<mapped_type>>
			requires std::constructible_from<key_type, Key> && std::convertible_to<std::ranges::range_reference_t<Rng>, mapped_type>
		constexpr auto insert_chunk_front(const Key& key, Rng&& rng){
			auto itr = std::ranges::lower_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert_range(itr, rng | std::views::transform([&](auto&& rng_value){
				return value_type{key_type{key}, std::forward_like<Rng>(rng_value)};
			}));
		}

		template <typename Key, std::ranges::input_range Rng = std::initializer_list<mapped_type>>
			requires std::constructible_from<key_type, Key> && std::convertible_to<std::ranges::range_reference_t<Rng>, mapped_type>
		constexpr auto insert_chunk_back(const Key& key, Rng&& rng){
			auto itr = std::ranges::upper_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert_range(itr, rng | std::views::transform([&](auto&& rng_value){
				return value_type{key_type{key}, std::forward_like<Rng>(rng_value)};
			}));
		}

		template <typename Key, typename... Args>
			requires std::constructible_from<key_type, std::remove_cvref_t<Key>> && std::constructible_from<mapped_type, Args...>
		constexpr auto insert_chunk_front(Key&& key, Args&&... value){
			auto itr = std::ranges::lower_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert(itr, flat_seq_map_storage{key_type{std::forward<Key>(key)}, mapped_type{std::forward<Args>(value)...}});
		}

		template <typename Key, typename... Args>
			requires std::constructible_from<key_type, std::remove_cvref_t<Key>> && std::constructible_from<mapped_type, Args...>
		constexpr auto insert_chunk_back(Key&& key, Args&&... value){
			auto itr = std::ranges::upper_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			return values.insert(itr, flat_seq_map_storage{key_type{std::forward<Key>(key)}, mapped_type{std::forward<Args>(value)...}});
		}

		template <typename Key, typename... Args>
			requires std::constructible_from<key_type, std::remove_cvref_t<Key>> && std::constructible_from<mapped_type, Args...>
		constexpr auto insert_unique(Key&& key, Args&&... value){
			auto itr = std::ranges::lower_bound(values, flat_seq_map::try_to_underlying(key), comp{},
			                                    flat_seq_map::get_key_proj);
			if(itr == values.end() || comp{}(flat_seq_map::try_to_underlying(key), itr->key)){
				return std::make_pair(values.insert(itr, flat_seq_map_storage{key_type{std::forward<Key>(key)}, mapped_type{std::forward<Args>(value)...}}), true);
			}

			return std::make_pair(itr, false);
		}

		template <typename S, typename Key>
		constexpr auto* find_unique(this S&& self, const Key& key) noexcept{
			auto rng = std::ranges::equal_range(std::forward_like<S>(self.values),
			                                flat_seq_map::try_to_underlying(std::as_const(key)), comp{},
			                                flat_seq_map::get_key_proj);
			if(std::ranges::empty(rng)){
				return static_cast<mapped_type*>(nullptr);
			}else{
				return std::addressof(std::ranges::begin(rng)->value);
			}
		}

		template <typename S, typename Key>
		constexpr auto find(this S&& self, const Key& key) noexcept{
			return std::ranges::equal_range(std::forward_like<S>(self.values),
			                                flat_seq_map::try_to_underlying(std::as_const(key)), comp{},
			                                flat_seq_map::get_key_proj) | std::views::transform(&value_type::value);
		}

		template <typename S, typename Key>
		constexpr auto find_front(this S&& self, const Key& key) noexcept{
			auto itr_begin = std::ranges::find_if_not(self.values, [&](const key_type& k){
				return comp{}(k, flat_seq_map::try_to_underlying(key));
			}, flat_seq_map::get_key_proj);

			auto itr_end = std::ranges::find_if(itr_begin, self.values.end(), [&](const key_type& k){
				return comp{}(flat_seq_map::try_to_underlying(key), k);
			}, flat_seq_map::get_key_proj);

			return std::ranges::borrowed_subrange_t<decltype(std::forward_like<S>(self.values))>{itr_begin, itr_end} |
				std::views::transform(&value_type::value);
		}

		template <typename S, typename Key>
		constexpr auto find_back(this S&& self, const Key& key) noexcept{
			auto itr_begin = std::ranges::find_if_not(self.values.rbegin(), self.values.rend(), [&](const key_type& k){
				return comp{}(flat_seq_map::try_to_underlying(key), k);
			}, flat_seq_map::get_key_proj);

			auto itr_end = std::ranges::find_if(itr_begin, self.values.rend(), [&](const key_type& k){
				return comp{}(k, flat_seq_map::try_to_underlying(key));
			}, flat_seq_map::get_key_proj);

			return std::ranges::borrowed_subrange_t<decltype(std::forward_like<S>(self.values))>{
					itr_end.base(), itr_begin.base()
				} | std::views::transform(&value_type::value);
		}

		template <std::invocable<const key_type&, mapped_type&> Fn>
		void each(Fn fn){
			for (auto && value : values){
				std::invoke(fn, value.key, value.value);
			}
		}
	};

	template <typename K, typename V, typename Comp>
	struct flat_seq_span{
	private:
		using key_type = K;
		using mapped_type = V;
		using comp = Comp;

		using cont = flat_seq_map<K, V, Comp>;
		using span = std::span<const typename cont::value_type>;
		using value_type = cont::value_type;

		span values;
	public:
		[[nodiscard]] flat_seq_span() = default;

		[[nodiscard]] explicit(false) flat_seq_span(const cont& seq)
			: values(seq.values){
		}

		template <typename S>
		constexpr auto find(this S&& self, const key_type& key) noexcept{
			return std::ranges::equal_range(std::forward_like<S>(self.values),
											cont::try_to_underlying(std::as_const(key)), comp{},
											cont::get_key_proj) | std::views::transform(&value_type::value);
		}

		template <typename S>
		constexpr auto find_front(this S&& self, const key_type& key) noexcept{
			auto itr_begin = std::ranges::find_if_not(self.values, [&](const key_type& k){
				return comp{}(k, cont::try_to_underlying(key));
			}, cont::get_key_proj);

			auto itr_end = std::ranges::find_if(itr_begin, self.values.end(), [&](const key_type& k){
				return comp{}(cont::try_to_underlying(key), k);
			}, cont::get_key_proj);

			return std::ranges::borrowed_subrange_t<decltype(std::forward_like<S>(self.values))>{itr_begin, itr_end} |
				std::views::transform(&value_type::value);
		}

		template <typename S>
		constexpr auto find_back(this S&& self, const key_type& key) noexcept{
			auto itr_begin = std::ranges::find_if_not(self.values.rbegin(), self.values.rend(), [&](const key_type& k){
				return comp{}(cont::try_to_underlying(key), k);
			}, cont::get_key_proj);

			auto itr_end = std::ranges::find_if(itr_begin, self.values.rend(), [&](const key_type& k){
				return comp{}(k, cont::try_to_underlying(key));
			}, cont::get_key_proj);

			return std::ranges::borrowed_subrange_t<decltype(std::forward_like<S>(self.values))>{
				itr_end.base(), itr_begin.base()
			} | std::views::transform(&value_type::value);
		}
	};
}

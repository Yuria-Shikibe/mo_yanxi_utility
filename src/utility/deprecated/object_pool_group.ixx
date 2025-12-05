module;

#include <cassert>

export module ext.object_pool_group;

export import mo_yanxi.object_pool;
export import mo_yanxi.open_addr_hash_map;
import std;

namespace mo_yanxi{
	struct type_index_equal_to__not_null{
		using is_transparent = void;
		bool operator()(std::nullptr_t, const std::type_index b) const noexcept {
			return b == typeid(std::nullptr_t);
		}

		bool operator()(const std::type_index b, std::nullptr_t) const noexcept {
			return b == typeid(std::nullptr_t);
		}

		bool operator()(const std::type_index b, const std::type_index a) const noexcept {
			return a == b;
		}

		constexpr bool operator()(std::nullptr_t, std::nullptr_t) const noexcept {
			return true;
		}
	};

	struct type_index_hasher{
		using is_transparent = void;
		static constexpr std::hash<std::type_index> hasher{};

		std::size_t operator()(const std::type_index val) const noexcept {
			return hasher(val);
		}

		std::size_t operator()(std::nullptr_t) const noexcept {
			return hasher(typeid(std::nullptr_t));
		}
	};

	struct Proj{
		std::type_index operator ()(std::nullptr_t) const noexcept{
			return typeid(std::nullptr_t);
		}
	};

	// using T = int

	export
	struct dynamic_object_pool_group{
		constexpr static std::size_t pool_size = sizeof(single_thread_object_pool<std::nullptr_t>);
		constexpr static std::size_t align = alignof(single_thread_object_pool<std::nullptr_t>);

		struct pool_wrapper{
			using deleter = void(pool_wrapper*) noexcept;
			using swapper_func = void(pool_wrapper&, pool_wrapper&&) noexcept;

		private:
			alignas(align) std::array<std::byte, pool_size> buffer;
			deleter* destructor{};
			swapper_func* move_constructor_func{};

		public:
			template <typename T>
			[[nodiscard]] constexpr explicit(false) pool_wrapper(std::in_place_type_t<T>) :
			destructor(+[](pool_wrapper* p) noexcept {
				auto* ptr = reinterpret_cast<single_thread_object_pool<T>*>(p->buffer.data());
				std::destroy_at(ptr);
			}), move_constructor_func(+[](pool_wrapper& self, pool_wrapper&& other) noexcept{
				auto* l = reinterpret_cast<single_thread_object_pool<T>*>(self.buffer.data());
				auto* r = reinterpret_cast<single_thread_object_pool<T>*>(other.buffer.data());
				std::construct_at(l, std::move(*r));
			}){
				auto* ptr = reinterpret_cast<single_thread_object_pool<T>*>(buffer.data());
				std::construct_at(ptr);
			}

			explicit operator bool() const noexcept{
				return destructor != nullptr;
			}

			template <typename T>
			[[nodiscard]] constexpr single_thread_object_pool<T>& as() noexcept{
				return reinterpret_cast<single_thread_object_pool<T>&>(buffer);
			}

			[[nodiscard]] constexpr pool_wrapper() = default;

			~pool_wrapper(){
				if(destructor){
					destructor(this);
				}
			}

			pool_wrapper(const pool_wrapper& other) = delete;

			pool_wrapper(pool_wrapper&& other) noexcept :
				move_constructor_func(std::exchange(other.move_constructor_func, {})){
				if(move_constructor_func){
					destructor = other.destructor;
					move_constructor_func(*this, std::move(other));
				}
			}

			pool_wrapper& operator=(pool_wrapper&& other) noexcept{
				if(this == &other)return *this;

				if(destructor){
					destructor(this);
				}

				move_constructor_func = std::exchange(other.move_constructor_func, {});
				if(move_constructor_func){
					destructor = other.destructor;
					move_constructor_func(*this, std::move(other));
				}else{
					destructor = nullptr;
				}

				return *this;
			}

			pool_wrapper& operator=(const pool_wrapper& other) noexcept = delete;
		};

	private:
		fixed_open_hash_map<
			std::type_index,
			pool_wrapper,
			nullptr,
			type_index_hasher, type_index_equal_to__not_null, Proj>
		pools{};

	public:
		template <typename T>
		single_thread_object_pool<T>& pool_of(){
			return pools.try_emplace(std::type_index{typeid(T)}, std::in_place_type<T>).first->second.template as<T>();
		}

		template <typename T, typename... Args>
		auto obtain_raw(/*std::in_place_type_t<T>, */Args&&... args){
			single_thread_object_pool<T>& pool = pool_of<T>();
			return pool.template obtain_raw<Args...>(std::forward<Args>(args)...);
		}

		template <typename T, typename... Args>
		auto obtain_unique(/*std::in_place_type_t<T>, */Args&&... args){
			single_thread_object_pool<T>& pool = pool_of<T>();
			return pool.template obtain_unique<Args...>(std::forward<Args>(args)...);
		}

		template <typename T, typename... Args>
		auto obtain_shared(/*std::in_place_type_t<T>, */Args&&... args){
			single_thread_object_pool<T>& pool = pool_of<T>();
			return pool.template obtain_shared<Args...>(std::forward<Args>(args)...);
		}
	};

	export
	template <typename ...Ts>
	struct static_object_pool_group{
	private:
		std::tuple<single_thread_object_pool<Ts> ...> pools{};

	public:
		template <typename T>
		single_thread_object_pool<T>& pool_of() noexcept{
			return std::get<single_thread_object_pool<T>>(pools);
		}

		template <typename T, typename ...Args>
		auto obtain_raw(/*std::in_place_type_t<T>, */Args&&... args){
			single_thread_object_pool<T>& pool = pool_of<T>();
			return pool.template obtain_raw<Args...>(std::forward<Args>(args)...);
		}

		template <typename T, typename ...Args>
		auto obtain_unique(/*std::in_place_type_t<T>, */Args&&... args){
			single_thread_object_pool<T>& pool = pool_of<T>();
			return pool.template obtain_unique<Args...>(std::forward<Args>(args)...);
		}

		template <typename T, typename ...Args>
		auto obtain_shared(/*std::in_place_type_t<T>, */Args&&... args){
			single_thread_object_pool<T>& pool = pool_of<T>();
			return pool.template obtain_shared<Args...>(std::forward<Args>(args)...);
		}
	};
}
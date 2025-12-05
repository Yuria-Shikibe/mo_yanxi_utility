module;

#ifdef MO_YANXI_UTILITY_ENABLE_CHECK
#define RTTI_CHECK
#include <cassert>
#endif

export module mo_yanxi.fixed_size_type_storage;

import std;

namespace mo_yanxi{
	export
	template <std::size_t align, std::size_t size>
	struct fixed_size_type_wrapper{
		using deleter = void(fixed_size_type_wrapper*) noexcept;

	private:
		using swapper_func = void(fixed_size_type_wrapper&, fixed_size_type_wrapper&&) noexcept;
		alignas(align) std::array<std::byte, size> buffer;
		deleter* destructor{};
		swapper_func* move_constructor_func{};

#ifdef RTTI_CHECK
		std::type_index type_idx{typeid(nullptr)};
#endif


	public:
		template <typename T, typename ...Args>
			requires (alignof(T) <= align && sizeof(T) <= size)
		[[nodiscard]] constexpr explicit(false) fixed_size_type_wrapper(std::in_place_type_t<T>, Args&& ...args) :
			destructor(+[](fixed_size_type_wrapper* p) noexcept{
				auto* ptr = reinterpret_cast<T*>(p->buffer.data());
				std::destroy_at(ptr);
			}), move_constructor_func(+[](fixed_size_type_wrapper& self, fixed_size_type_wrapper&& other) noexcept{
				auto* l = reinterpret_cast<T*>(self.buffer.data());
				auto* r = reinterpret_cast<T*>(other.buffer.data());
				std::construct_at(l, std::move(*r));
			}){
			auto* ptr = reinterpret_cast<T*>(buffer.data());

#ifdef RTTI_CHECK
			type_idx = typeid(std::remove_cvref_t<T>);
#endif

			std::construct_at(ptr, std::forward<Args>(args) ...);
		}

		explicit operator bool() const noexcept{
			return destructor != nullptr;
		}

		template <typename T>
		[[nodiscard]] constexpr T& as() noexcept{
#ifdef RTTI_CHECK
			assert(type_idx == typeid(std::remove_cvref_t<T>));
#endif

			return reinterpret_cast<T&>(buffer);
		}

		[[nodiscard]] constexpr fixed_size_type_wrapper() = default;

		~fixed_size_type_wrapper(){
			destroy();
		}

		fixed_size_type_wrapper(const fixed_size_type_wrapper& other) = delete;

		fixed_size_type_wrapper(fixed_size_type_wrapper&& other) noexcept :
			move_constructor_func(std::exchange(other.move_constructor_func, {})){
			if(move_constructor_func){
				destructor = other.destructor;
				move_constructor_func(*this, std::move(other));

#ifdef RTTI_CHECK
				type_idx = other.type_idx;
#endif
			}
		}

		fixed_size_type_wrapper& operator=(fixed_size_type_wrapper&& other) noexcept{
			if(this == &other) return *this;

			destroy();

			move_constructor_func = std::exchange(other.move_constructor_func, {});
			if(move_constructor_func){
				destructor = other.destructor;
				move_constructor_func(*this, std::move(other));

#ifdef RTTI_CHECK
				type_idx = other.type_idx;
#endif

			} else{
				destructor = nullptr;

#ifdef RTTI_CHECK
				type_idx = typeid(nullptr);
#endif
			}

			return *this;
		}

		fixed_size_type_wrapper& operator=(const fixed_size_type_wrapper& other) noexcept = delete;

	private:
		void destroy(){
			if(destructor){

#ifdef RTTI_CHECK
				assert(type_idx != typeid(nullptr));
#endif

				destructor(this);
			}
		}
	};
}

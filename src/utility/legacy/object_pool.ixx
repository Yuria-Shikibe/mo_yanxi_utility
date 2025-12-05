module;

#include <vcruntime_typeinfo.h>

#include "../../../include/mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.object_pool;

import mo_yanxi.array_stack;
import ext.atomic_caller;
import std;

namespace mo_yanxi{

	template <typename T, std::size_t count>
	struct single_thread_object_pool_page{
		T* data_uninitialized;

		array_stack<T*, count> valid_pointers{};

		[[nodiscard]] single_thread_object_pool_page() = default;

		[[nodiscard]] constexpr explicit single_thread_object_pool_page(T* data) : data_uninitialized{data}{
			for(std::size_t i = 0; i < count; ++i){
				valid_pointers.push(data_uninitialized + i);
			}
		}

		/** @brief DEBUG USAGE */
		[[nodiscard]] constexpr std::span<T> as_span() const noexcept{
			return std::span{data_uninitialized, count};
		}

		template <std::invocable<single_thread_object_pool_page&> Func>
		FORCE_INLINE decltype(auto) lock_and(Func func){
			return func(*this);
		}

		template<typename Alloc>
		FORCE_INLINE constexpr void free(Alloc& alloc) const noexcept{

// #if MO_YANXI_UTILITY_ENABLE_CHECK
			if(!valid_pointers.full()){
				std::cerr << std::format("pool<{}> leaked: {}/{}\n", typeid(T).name(), valid_pointers.size(), count);
				// throw std::runtime_error("pool_page::free: not all pointers are stored");
			}
// #endif

			std::allocator_traits<Alloc>::deallocate(alloc, data_uninitialized, count);
		}

		FORCE_INLINE constexpr void store(T* p) noexcept(std::is_nothrow_destructible_v<T>){
			if constexpr(!std::is_trivially_destructible_v<T>){
				std::destroy_at(p);
			}

			assert(!valid_pointers.full());
			valid_pointers.push(p);
		}

		/**
		 * @return nullptr if the page is empty, or uninitialized pointer
		 */
		constexpr T* borrow_uninitialized() noexcept{
			if(valid_pointers.empty()){
				return nullptr;
			}

			T* p = valid_pointers.back();
			valid_pointers.pop();

			return p;
		}

		//warning : the resource manage is tackled by external call

		single_thread_object_pool_page(const single_thread_object_pool_page& other) = delete;
		constexpr single_thread_object_pool_page(single_thread_object_pool_page&& other) noexcept = delete;
		single_thread_object_pool_page& operator=(const single_thread_object_pool_page& other) = delete;
		constexpr single_thread_object_pool_page& operator=(single_thread_object_pool_page&& other) noexcept = delete;
	};

	template <typename T, std::size_t count>
	struct single_thread_pool_deleter{
		single_thread_object_pool_page<T, count>* page{};

		[[nodiscard]] single_thread_pool_deleter() = default;

		[[nodiscard]] explicit single_thread_pool_deleter(single_thread_object_pool_page<T, count>* page)
			: page{page}{}

		void operator()(T* ptr) const noexcept{
			if(ptr)page->store(ptr);
		}
	};

	export
	/**
	 * @brief a really simple object pool
	 * @tparam T type
	 * @tparam PageSize maximum objects a page can contain
	 * @tparam Alloc allocator type ONLY SUPPORTS DEFAULT ALLOCATOR NOW
	 */
	template <typename T, std::size_t PageSize = 512>
	struct single_thread_object_pool{
		using value_type = T;

		using page_type = single_thread_object_pool_page<T, PageSize>;
		using deleter_type = single_thread_pool_deleter<T, PageSize>;
		using unique_ptr = std::unique_ptr<T, deleter_type>;
		using allocator_type = std::allocator<T>;

		struct obtain_return_type{
			value_type* data;
			page_type* page;
		};

	private:
		std::list<page_type> pages{};

		ADAPTED_NO_UNIQUE_ADDRESS
		allocator_type allocator{};

	public:
		template <typename... Args>
			requires (std::constructible_from<T, Args...>)
		[[nodiscard]] unique_ptr obtain_unique(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>){
			auto [ptr, page] = this->obtain_raw(std::forward<Args>(args)...);
			return unique_ptr{ptr, deleter_type{page}};
		}

		template <typename... Args>
			requires (std::constructible_from<T, Args...>)
		[[nodiscard]] std::shared_ptr<T> obtain_shared(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>){
			auto [ptr, page] = this->template obtain_raw<Args...>(std::forward<Args>(args)...);
			return std::shared_ptr<T>{ptr, deleter_type{page}};
		}

		template <typename... Args>
			requires (std::constructible_from<T, Args...>)
		[[nodiscard]] obtain_return_type obtain_raw(Args&&... args)
			noexcept(std::is_nothrow_constructible_v<T, Args...>){
			obtain_return_type rstPair{};

			for(page_type& page : pages){
				rstPair.data = page.borrow_uninitialized();
				if(rstPair.data){
					rstPair.page = &page;
					goto ret;
				}
			}

			if(rstPair.data == nullptr){
				T* ptr = std::allocator_traits<allocator_type>::allocate(allocator, PageSize);
				rstPair.page = &pages.emplace_back(ptr);
				rstPair.data = rstPair.page->borrow_uninitialized();
			}

			ret:

			rstPair.data = std::construct_at(rstPair.data, std::forward<Args>(args)...);

			return rstPair;
		}

		/** @brief DEBUG USAGE */
		[[nodiscard]] const std::list<page_type>& get_pages() const noexcept{
			return pages;
		}

		[[nodiscard]] single_thread_object_pool() = default;

		~single_thread_object_pool(){
			for (const auto & page : pages){
				page.free(allocator);
			}
		}

		single_thread_object_pool(const single_thread_object_pool& other) = delete;

		single_thread_object_pool(single_thread_object_pool&& other) noexcept : pages(std::exchange(other.pages, {})),
			allocator(std::exchange(other.allocator, {})){
		}

		single_thread_object_pool& operator=(const single_thread_object_pool& other) = delete;

		//TODO correct move for allocators
		single_thread_object_pool& operator=(single_thread_object_pool&& other) noexcept{
			if(this == &other) return *this;
			std::swap(allocator, other.allocator);
			std::swap(pages, other.pages);
			return *this;
		}
	};
}

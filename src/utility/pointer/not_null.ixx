module;

#include <cassert>

export module ext.not_null;

import std;

namespace mo_yanxi{
	export
	template <typename T>
	struct not_null_ptr{
		[[nodiscard]] constexpr not_null_ptr() = default;

		[[nodiscard]] explicit(false) constexpr not_null_ptr(T& ptr) noexcept
			: ptr(std::addressof(ptr)){
		}

		constexpr friend bool operator==(const not_null_ptr& lhs, const not_null_ptr& rhs) noexcept = default;
		constexpr auto operator<=>(const not_null_ptr& rhs) const noexcept = default;

		constexpr not_null_ptr& operator=(T& obj) noexcept{
			this->ptr = std::addressof(obj);
			return *this;
		}

		constexpr not_null_ptr& operator=(T* obj) noexcept{
			this->ptr = obj;
			return *this;
		}

		constexpr explicit(false) operator T&() const noexcept{
			return *ptr;
		}

		constexpr T* operator->() const noexcept{
			assert(ptr != nullptr);
			return ptr;
		}

		[[nodiscard]] constexpr T* get() const noexcept{
			return ptr;
		}

		constexpr T& operator*() const noexcept{
			assert(ptr != nullptr);
			return *ptr;
		}

		constexpr explicit operator bool() const noexcept{
			assert(ptr != nullptr);
			return ptr != nullptr;
		}

	private:
		T* ptr{};
	};
}


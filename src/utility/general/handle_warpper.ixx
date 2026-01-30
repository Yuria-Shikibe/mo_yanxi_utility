module;

#include <cassert>

export module mo_yanxi.handle_wrapper;
import std;

export namespace mo_yanxi{
	template <typename T, bool enableCopy = false>
		requires std::is_pointer_v<T>
	struct wrapper{
	protected:
		T handle{};

	public:
		using type = T;

		[[nodiscard]] constexpr wrapper() = default;

		[[nodiscard]] constexpr explicit(false) wrapper(const T handler)
			: handle{handler}{}

		constexpr T release() noexcept{
			return std::exchange(handle, nullptr);
		}

		constexpr wrapper(const wrapper& other) noexcept requires (enableCopy) = default;

		constexpr wrapper& operator=(const wrapper& other) noexcept requires (enableCopy) = default;

		constexpr wrapper(wrapper&& other) noexcept
			: handle{std::exchange(other.handle, {})}{
		}

		constexpr wrapper& operator=(wrapper&& other) noexcept{
			if(this == &other) return *this;
			std::swap(handle, other.handle);
			return *this;
		}

	protected:
		constexpr wrapper& operator=(T other) noexcept{
			handle = other;
			return *this;
		}

	public:
		template <typename Ty>
		[[nodiscard]] constexpr decltype(auto) operator*(this Ty&& self) noexcept(
			requires(T t){ requires noexcept(*std::forward_like<Ty>(t)); }){
			assert(static_cast<bool>(self));
			return *std::forward_like<Ty>(self.handle);
		}

		[[nodiscard]] constexpr explicit(false) operator T() const noexcept{ return handle; }

		[[nodiscard]] constexpr explicit operator bool() const noexcept{ return handle != nullptr; }

		[[nodiscard]] constexpr T operator->() const noexcept{ return handle; }

		[[nodiscard]] constexpr T get() const noexcept{ return handle; }

		[[nodiscard]] constexpr std::array<T, 1> as_seq() const noexcept{ return std::array{handle}; }

		[[nodiscard]] constexpr const T* as_data() const noexcept{ return &handle; }

		[[nodiscard]] constexpr T* as_data() noexcept{ return &handle; }
	};
	/**
	 * @brief Make move constructor default declarable
	 * @tparam T Dependency Type
	 */
	template <typename T>
	struct dependency{
		using type = T;

		T handle{};

		[[nodiscard]] constexpr dependency() noexcept = default;

		[[nodiscard]] constexpr explicit(false) dependency(const T& device) noexcept : handle{device}{}

		constexpr ~dependency() = default;

		[[nodiscard]] constexpr explicit(false) operator T() const noexcept
			requires (std::is_scalar_v<T>)
		{
			return handle;
		}

		[[nodiscard]] constexpr explicit(false) operator const T&() const noexcept
			requires (!std::is_scalar_v<T>)
		{
			return handle;
		}

		[[nodiscard]] constexpr explicit operator bool() const noexcept
			requires requires(const T& h){
				static_cast<bool>(h);
			}{
			return static_cast<bool>(handle);
		}

		[[nodiscard]] constexpr auto operator->() const noexcept{
			if constexpr (std::is_pointer_v<T>){
				return handle;
			}else{
				return std::addressof(handle);
			}
		}

		template <typename Ty>
		[[nodiscard]] constexpr decltype(auto) operator*(this Ty&& self) noexcept(
			requires(T t){ requires noexcept( *std::forward_like<Ty>(t)); }) requires requires(Ty&& s){ *std::forward_like<Ty>(s); }{
			assert(static_cast<bool>(self));
			return *std::forward_like<Ty>(self.handle);
		}

		[[nodiscard]] constexpr const T* as_data() const noexcept{ return &handle; }
		[[nodiscard]] constexpr T* as_data() noexcept{ return &handle; }

		[[nodiscard]] constexpr T get() const noexcept requires(std::is_scalar_v<T>) {
			return handle;
		}

		constexpr dependency(const dependency& other) = delete;

		constexpr dependency(dependency&& other) noexcept
			: handle{std::exchange(other.handle, T{})}{
		}

		constexpr dependency& operator=(const dependency& other) = delete;

		constexpr dependency& operator=(dependency&& other) noexcept(std::is_nothrow_swappable_v<T>)
			requires (std::swappable<T>)
		{
			if(this == &other) return *this;
			std::ranges::swap(handle, other.handle);
			return *this;
		}

		constexpr friend bool operator==(const dependency& lhs, const dependency& rhs) noexcept requires (std::equality_comparable<T>) = default;
		constexpr friend bool operator==(const dependency& lhs, const T& rhs) noexcept requires (std::equality_comparable<T>){
			return lhs.handle == rhs;
		}constexpr friend bool operator==(const T& lhs, const dependency& rhs) noexcept requires (std::equality_comparable<T>){
			return rhs.handle == lhs;
		}
	};

	template <typename T>
	using exclusive_handle_member = dependency<T>;

	template <typename T>
	using exclusive_handle = wrapper<T>;
}

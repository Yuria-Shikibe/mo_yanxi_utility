export module mo_yanxi.string_array;

import std;

namespace mo_yanxi {
    template <unsigned long long N>
    struct get_smallest_uint {
        using type =
            std::conditional_t<(N <= 0xFF), std::uint8_t,
            std::conditional_t<(N <= 0xFFFF), std::uint16_t,
            std::conditional_t<(N <= 0xFFFFFFFF), std::uint32_t,
            std::uint64_t>>>;
    };

    template <unsigned long long N>
    using smallest_uint_t = typename get_smallest_uint<N>::type;

    export
    template <std::size_t N>
    class static_string {
        static_assert(N > 0, "Capacity must be greater than 0");
    public:
        static constexpr std::size_t max_size = N - 1;

        using size_type = smallest_uint_t<N>;

        constexpr static_string() noexcept : m_data{ 0 } {}

        constexpr static_string(const char* str) noexcept {
            assign(str);
        }

        constexpr static_string(std::string_view sv) noexcept {
            assign(sv);
        }

        // 核心修改：区分编译期和运行时的赋值逻辑
        constexpr void assign(std::string_view sv_truncate_exceed) noexcept {
            m_size = static_cast<size_type>(std::min(sv_truncate_exceed.length(), max_size));

            if consteval {
                // [编译期] 必须使用循环，因为 memcpy 不是 constexpr
                for (std::size_t i = 0; i < m_size; ++i) {
                    m_data[i] = sv_truncate_exceed[i];
                }
            }
            else {
            	std::memcpy(m_data, sv_truncate_exceed.data(), m_size);
            }

            // 强制 zero termination
            m_data[m_size] = '\0';
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return m_size;
        }

        [[nodiscard]] constexpr const char* c_str() const noexcept {
            return m_data;
        }

        [[nodiscard]] constexpr const char* data() const noexcept {
            return m_data;
        }

        constexpr operator std::string_view() const noexcept {
            return std::string_view(m_data, m_size);
        }

        // --- 新增：比较运算符 ---
        
        // 使用 string_view 进行三路比较 (C++20 spaceship operator)
        friend constexpr auto operator<=>(const static_string& lhs, const static_string& rhs) noexcept {
            return std::string_view(lhs) <=> std::string_view(rhs);
        }

        // 使用 string_view 进行相等性比较
        friend constexpr bool operator==(const static_string& lhs, const static_string& rhs) noexcept {
            return std::string_view(lhs) == std::string_view(rhs);
        }

    private:
        size_type m_size{};
        char m_data[N];
    };
}

// --- 新增：标准库特化 (std::formatter 和 std::hash) ---

export namespace std {
    // std::formatter 特化：直接继承自 formatter<string_view> 以支持标准格式化说明符
    template <std::size_t N>
    struct formatter<mo_yanxi::static_string<N>> : formatter<std::string_view> {
        template <typename FormatContext>
        auto format(const mo_yanxi::static_string<N>& s, FormatContext& ctx) const {
            return formatter<std::string_view>::format(std::string_view(s), ctx);
        }
    };

    // std::hash 特化：委托给 hash<string_view>
    template <std::size_t N>
    struct hash<mo_yanxi::static_string<N>> {
        std::size_t operator()(const mo_yanxi::static_string<N>& s) const noexcept {
            return std::hash<std::string_view>{}(std::string_view(s));
        }
    };
}
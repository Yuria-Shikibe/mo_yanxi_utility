export module mo_yanxi.static_string;

import mo_yanxi.meta_programming;
import std;

namespace mo_yanxi {
    export
    template <std::size_t N>
    class static_string {
        static_assert(N > 0, "Capacity must be greater than 0");
    public:
        static constexpr std::size_t max_size = N - 1;

        using size_type = smallest_uint_t<N>;

        constexpr static_string() noexcept = default;

        constexpr static_string(const char* str) {
            assign(str);
        }

        constexpr static_string(std::string_view sv) {
            assign(sv);
        }

        // 核心修改：区分编译期和运行时的赋值逻辑
        constexpr void assign(std::string_view sv) {
        	const auto src_len = sv.length();
        	if(src_len > max_size){
        		throw std::bad_alloc{};
        	}

            m_size = src_len;

            if consteval {
                // [编译期] 必须使用循环，因为 memcpy 不是 constexpr
                for (std::size_t i = 0; i < m_size; ++i) {
                    m_data[i] = sv[i];
                }
            }
            else {
            	std::memcpy(m_data, sv.data(), m_size);
            }

            // 强制 zero termination
            m_data[m_size] = '\0';
        }

    	constexpr void assign_or_truncate(std::string_view sv) noexcept {
        	m_size = static_cast<size_type>(std::min(sv.length(), max_size));;

        	if consteval {
        		// [编译期] 必须使用循环，因为 memcpy 不是 constexpr
        		for (std::size_t i = 0; i < m_size; ++i) {
        			m_data[i] = sv[i];
        		}
        	}
        	else {
        		std::memcpy(m_data, sv.data(), m_size);
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

        constexpr auto get_data() const noexcept -> const char(&)[N]{
	        return m_data;
        }


    private:
        size_type m_size{};
        char m_data[N]{};
    };

export
template <std::size_t Capacity, std::size_t Alignment = alignof(char)>
class array_string {
public:
	static_assert(Capacity > 0);
    static_assert(std::has_single_bit(Alignment), "Alignment must be a power of 2");

    using value_type = char;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = char*;
    using const_pointer = const char*;
    using iterator = char*;
    using const_iterator = const char*;

    // =========================================================
    // 构造函数
    // =========================================================

    /**
     * @brief 默认构造：零初始化缓冲区 (空字符串)
     */
    constexpr array_string() noexcept = default;

    /**
     * @brief 从 std::string_view 构造，超出容量截断
     */
    constexpr array_string(std::string_view sv) noexcept {
        assign(sv);
    }

    /**
     * @brief 从 C 风格字符串构造
     */
    constexpr array_string(const char* str) noexcept {
        assign(std::string_view(str));
    }

	constexpr auto get_data() const noexcept -> const char(&)[Capacity]{
		return m_data;
    }


    // =========================================================
    // 赋值与核心逻辑
    // =========================================================

    constexpr void assign(std::string_view sv) noexcept {
        // 计算本次需要复制的长度 (不存储到成员变量，仅作局部使用)
        const size_type count = std::min(sv.size(), Capacity - 1);

        if (count > 0) {
            if consteval {
                // 编译期：手动循环
                for (size_type i = 0; i < count; ++i) {
                    m_data[i] = sv[i];
                }
            } else {
                // 运行时：memcpy
                std::memcpy(m_data, sv.data(), count);
            }
        }

        // 必须显式设置 Null 终止符，因为不再存储 size
        // 后续逻辑完全依赖这个 \0 来判断结束
        m_data[count] = '\0';
    }

    constexpr array_string& operator=(std::string_view sv) noexcept {
        assign(sv);
        return *this;
    }

    // =========================================================
    // 观察器 (Observers)
    // =========================================================

    /**
     * @brief 获取当前容量
     */
    [[nodiscard]] constexpr size_type capacity() const noexcept { return Capacity - 1; }

    /**
     * @brief 获取原始指针
     */
    [[nodiscard]] constexpr const_pointer data() const noexcept { return m_data; }
    [[nodiscard]] constexpr const_pointer c_str() const noexcept { return m_data; }
    [[nodiscard]] constexpr pointer data() noexcept { return m_data; }

    /**
     * @brief 计算字符串长度 (O(N) 复杂度)
     * 依赖 std::string_view 的 constexpr 构造来寻找 \0
     */
    [[nodiscard]] constexpr size_type size() const noexcept {
        return std::string_view(m_data).size();
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return m_data[0] == '\0';
    }

    // 隐式转换与视图
    constexpr operator std::string_view() const noexcept {
        return std::string_view(m_data); // 自动扫描 \0 计算长度
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return std::string_view(m_data);
    }

    // =========================================================
    // 迭代器 (需动态计算 end)
    // =========================================================

    [[nodiscard]] constexpr iterator begin() noexcept { return m_data; }
    [[nodiscard]] constexpr const_iterator begin() const noexcept { return m_data; }

    // end() 需要遍历找到 \0
    [[nodiscard]] constexpr iterator end() noexcept { return m_data + size(); }
    [[nodiscard]] constexpr const_iterator end() const noexcept { return m_data + size(); }

    // =========================================================
    // 比较操作
    // =========================================================

    constexpr auto operator<=>(const array_string& other) const noexcept {
        return view() <=> other.view();
    }

    constexpr auto operator<=>(std::string_view sv) const noexcept {
        return view() <=> sv;
    }

    constexpr bool operator==(std::string_view sv) const noexcept {
        return view() == sv;
    }

private:
    // 仅存储数据，Capacity + 1 保证 '\0' 空间
    // 默认初始化 {} 保证初始全为 0
    alignas(Alignment) char m_data[Capacity]{};
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
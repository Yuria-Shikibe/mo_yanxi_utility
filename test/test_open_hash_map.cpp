#include <gtest/gtest.h>
import mo_yanxi.open_addr_hash_map;

using namespace mo_yanxi;

struct ConstexprStringHasher {
    using is_transparent = void; // 支持异质查找

    [[nodiscard]] static constexpr std::size_t operator()(std::string_view sv) noexcept {
        std::size_t hash = 5381;
        for (char c : sv) {
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(c); /* hash * 33 + c */
        }
        return hash;
    }
};

// 定义一个包含非平凡对象（std::string）的结构体
struct UserProfile {
    std::string username;
    std::string bio;
    int level;

    // 默认比较运算符
    constexpr bool operator==(const UserProfile&) const = default;
};

// 强制在编译期执行的测试函数
consteval bool verify_compile_time_map() {
    struct empty_str_trans{
        static constexpr std::string operator ()(std::nullptr_t) noexcept{
            return std::string();
        }
    };

    // 1. 定义 Map，使用 std::string 作为 Key，UserProfile 作为 Value
    // 使用自定义的 constexpr Hasher
    using MapType = fixed_open_hash_map<
        std::string,
        UserProfile,
        nullptr, // 使用默认的 empty key 标记
        ConstexprStringHasher,
        std::equal_to<std::string>,
        empty_str_trans
    >;

    using namespace std::literals;

    MapType map(8); // 初始容量

    // 2. 插入数据 (测试 emplace 和非平凡对象的构造)
    // "Alice" 需要动态分配内存（取决于实现，SSO 可能不分配，但逻辑是一样的）
    map.emplace("Alice"s, UserProfile{"Alice", "Developer", 10});
    map.emplace("Bob"s, UserProfile{"Bob", "Manager", 20});

    // 3. 查找与验证 (测试 find 和 operator->)
    auto it_alice = map.find("Alice"s);
    if (it_alice == map.end()) return false;
    if (it_alice->second.bio != "Developer"s) return false;

    // 4. 修改数据 (测试非平凡对象的赋值/析构)
    // 这里会触发旧值的析构 (std::string dtor) 和新值的移动构造
    map.insert_or_assign("Alice"s, UserProfile{"Alice", "Senior Developer", 11});

    if (map.at("Alice"s).level != 11) return false;

    // 5. 扩容测试 (测试 rehash 和数据迁移)
    // 插入更多数据以触发潜在的 rehash 或冲突
    map.emplace("Charlie"s, UserProfile{"Charlie", "Tester", 5});
    map.emplace("Dave"s, UserProfile{"Dave", "Designer", 8});
    map.emplace("Eve"s, UserProfile{"Eve", "Hacker", 99});

    if (map.size() != 5) return false;

    // 6. 删除测试 (测试 erase 和对象的析构)
    map.erase("Bob"s);
    if (map.contains("Bob"s)) return false;
    if (map.size() != 4) return false;

    // 7. 验证剩余数据完整性
    if (map.at("Eve"s).bio != "Hacker") return false;

    // 8. 清空测试
    map.clear();
    if (!map.empty()) return false;

    return true;
}

// 核心：使用 static_assert 触发编译期执行
// 如果 map 实现中有任何非 constexpr 的操作（如 reinterpret_cast, void* 转换等），
// 编译器会在这里直接报错。
static_assert(verify_compile_time_map(), "Compile-time hash map verification failed!");

TEST(OpenHashMapTest, BasicOperations) {
    fixed_open_hash_map<int, std::string> map;
    EXPECT_TRUE(map.empty());

    map.insert({1, "one"});
    EXPECT_EQ(map.size(), 1);
    EXPECT_TRUE(map.contains(1));
    EXPECT_EQ(map.at(1), "one");

    map[2] = "two";
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[2], "two");

    map.erase(1);
    EXPECT_FALSE(map.contains(1));
    EXPECT_EQ(map.size(), 1);

    map.clear();
    EXPECT_TRUE(map.empty());
}

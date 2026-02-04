#include <gtest/gtest.h>
import mo_yanxi.cache;

using namespace mo_yanxi;

consteval bool test_compile_time_lru() {
    // 1. 创建一个容量为 3 的缓存
    mo_yanxi::lru_cache<int, int, 3> cache;

    // 2. 填充数据
    cache.put(1, 100);
    cache.put(2, 200);
    cache.put(3, 300);

    // 验证插入
    if (cache.size() != 3) return false;
    if (*cache.get(1) != 100) return false; // 访问 key 1，使其成为最近使用 (MRU)

    // cache status: [1:100] -> [3:300] -> [2:200] (Most -> Least)

    // 3. 触发驱逐 (插入 4，应该驱逐最久未使用的 2)
    cache.put(4, 400);

    if (cache.contains(2)) return false; // 2 应该被删除了
    if (!cache.contains(4)) return false;
    if (!cache.contains(1)) return false; // 1 应该还在

    // 4. 验证值更新
    cache.put(4, 401);
    if (*cache.get(4) != 401) return false;

    // 5. 验证 Clear
    cache.clear();
    if (!cache.empty()) return false;

    return true;
}

// 编译期断言：如果 test_compile_time_lru() 返回 false 或无法编译，则构建失败
static_assert(test_compile_time_lru(), "LRU Cache Constexpr Test Failed!");


TEST(LRUCacheTest, BasicOperations) {
    lru_cache<int, std::string, 3> cache;
    EXPECT_TRUE(cache.empty());

    cache.put(1, "one");
    EXPECT_EQ(cache.size(), 1);
    EXPECT_TRUE(cache.contains(1));

    cache.put(2, "two");
    cache.put(3, "three");
    EXPECT_EQ(cache.size(), 3);

    // Access 1 to make it most recently used
    EXPECT_NE(cache.get(1), nullptr);

    // Add 4, should evict 2 (LRU)
    cache.put(4, "four");
    EXPECT_EQ(cache.size(), 3);
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(4));
    EXPECT_FALSE(cache.contains(2));

    // Update existing
    cache.put(1, "one_updated");
    EXPECT_EQ(*cache.get(1), "one_updated");

    cache.clear();
    EXPECT_TRUE(cache.empty());
}

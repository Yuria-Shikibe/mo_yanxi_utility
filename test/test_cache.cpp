#include <gtest/gtest.h>
import mo_yanxi.cache;

using namespace mo_yanxi;

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

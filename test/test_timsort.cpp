#include <gtest/gtest.h>
import mo_yanxi.algo.timsort;
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>

using namespace mo_yanxi::algo;

TEST(TimsortTest, Sort) {
    std::vector<int> v = {5, 2, 9, 1, 5, 6};
    timsort(v);

    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[5], 9);
}

TEST(TimsortTest, Random) {
    std::vector<int> v(100);
    std::iota(v.begin(), v.end(), 0);
    std::mt19937 g(42);
    std::shuffle(v.begin(), v.end(), g);

    timsort(v);
    EXPECT_TRUE(std::is_sorted(v.begin(), v.end()));
}

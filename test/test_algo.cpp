#include <gtest/gtest.h>
import mo_yanxi.algo;
#include <vector>
#include <ranges>

using namespace mo_yanxi;

TEST(AlgoTest, Accumulate) {
    std::vector<int> v = {1, 2, 3, 4, 5};
    auto sum = algo::accumulate(v, 0);
    EXPECT_EQ(sum, 15);
}

TEST(AlgoTest, PartBy) {
    std::vector<int> v = {1, 2, 3, 4, 5, 6};
    auto [even, odd] = algo::partBy(v, [](int x) { return x % 2 == 0; });

    int count = 0;
    for(auto x : even) { EXPECT_EQ(x % 2, 0); count++; }
    EXPECT_EQ(count, 3);

    count = 0;
    for(auto x : odd) { EXPECT_EQ(x % 2, 1); count++; }
    EXPECT_EQ(count, 3);
}

TEST(AlgoTest, RemoveUnstable) {
    std::vector<int> v = {1, 2, 3, 2, 4};
    auto rng = algo::remove_unstable(v, 2);
    // Unstable remove swaps with end.
    // algo::remove_unstable returns the subrange of "removed" elements (the tail).
    // The valid elements are [v.begin(), rng.begin()).

    // Check valid part
    for(auto it = v.begin(); it != rng.begin(); ++it) {
        EXPECT_NE(*it, 2);
    }

    // Check size of valid part (should be 3: 1, 3, 4)
    EXPECT_EQ(std::distance(v.begin(), rng.begin()), 3);

    // Check size of removed part
    EXPECT_EQ(rng.size(), 2);
}

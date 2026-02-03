#include <gtest/gtest.h>
import ext.views;
#include <vector>
#include <ranges>

using namespace mo_yanxi::ranges;

TEST(ExtViewsTest, SplitIf) {
    std::vector<int> v = {1, 2, 0, 3, 4, 0, 5};
    auto view = v | views::split_if([](int x) { return x == 0; });

    int chunks = 0;
    for(auto chunk : view) {
        chunks++;
        // Verify chunk size
        int size = 0;
        for(auto x : chunk) size++;
        if (chunks == 1) EXPECT_EQ(size, 2);
        if (chunks == 2) EXPECT_EQ(size, 2);
        if (chunks == 3) EXPECT_EQ(size, 1);
    }
    EXPECT_EQ(chunks, 3);
}

TEST(ExtViewsTest, PartIf) {
    std::vector<int> v = {1, 2, 0, 3, 4, 0, 5};
    auto view = v | views::part_if([](int x) { return x == 0; });

    int chunks = 0;
    for(auto chunk : view) {
        chunks++;
        int size = 0;
        for(auto x : chunk) size++;
        if (chunks == 1) EXPECT_EQ(size, 3); // 1, 2, 0
        if (chunks == 2) EXPECT_EQ(size, 3); // 3, 4, 0
        if (chunks == 3) EXPECT_EQ(size, 1); // 5
    }
    EXPECT_EQ(chunks, 3);
}

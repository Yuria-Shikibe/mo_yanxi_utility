#include <gtest/gtest.h>
import mo_yanxi.shared_stack;

using namespace mo_yanxi;

TEST(SharedStackTest, BasicOperations) {
    shared_stack<int> s(10);
    EXPECT_EQ(s.size(), 0);

    s.push(1);
    EXPECT_EQ(s.size(), 1);
    EXPECT_EQ(s[0], 1);

    s.push(2);
    EXPECT_EQ(s.size(), 2);
    EXPECT_EQ(s[1], 2);

    s.reset();
    EXPECT_EQ(s.capacity(), 0);
}

TEST(SharedStackTest, Emplace) {
    shared_stack<std::pair<int, int>> s(10);
    s.emplace(1, 2);
    EXPECT_EQ(s[0].first, 1);
    EXPECT_EQ(s[0].second, 2);
}

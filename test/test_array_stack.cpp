#include <gtest/gtest.h>
import mo_yanxi.array_stack;

using namespace mo_yanxi;

TEST(ArrayStackTest, BasicOperations) {
    array_stack<int, 5> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
    EXPECT_FALSE(s.full());

    s.push(1);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.size(), 1);
    EXPECT_EQ(s.back(), 1);

    s.push(2);
    EXPECT_EQ(s.back(), 2);
    EXPECT_EQ(s.size(), 2);

    s.pop();
    EXPECT_EQ(s.back(), 1);
    EXPECT_EQ(s.size(), 1);

    auto val = s.try_pop_and_get();
    EXPECT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);
    EXPECT_TRUE(s.empty());
}

TEST(ArrayStackTest, Clear) {
    array_stack<int, 5> s;
    s.push(1);
    s.push(2);
    s.clear();
    EXPECT_TRUE(s.empty());
}

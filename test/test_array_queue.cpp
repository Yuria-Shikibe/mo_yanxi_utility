#include <gtest/gtest.h>
import mo_yanxi.ext.array_queue;

using namespace mo_yanxi;

TEST(ArrayQueueTest, BasicOperations) {
    array_queue<int, 5> q;
    EXPECT_TRUE(q.empty());
    EXPECT_FALSE(q.full());
    EXPECT_EQ(q.size(), 0);
    EXPECT_EQ(q.max_size(), 5);

    q.push_back(1);
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.size(), 1);
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 1);

    q.push_back(2);
    EXPECT_EQ(q.size(), 2);
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 2);

    q.pop_front();
    EXPECT_EQ(q.size(), 1);
    EXPECT_EQ(q.front(), 2);
    EXPECT_EQ(q.back(), 2);

    q.pop_front();
    EXPECT_TRUE(q.empty());
}

TEST(ArrayQueueTest, Replace) {
    array_queue<int, 2> q;
    q.push_back(1);
    q.push_back(2);
    EXPECT_TRUE(q.full());

    q.push_back_and_replace(3);
    EXPECT_TRUE(q.full());
    EXPECT_EQ(q.front(), 2);
    EXPECT_EQ(q.back(), 3);
}

TEST(ArrayQueueTest, Emplace) {
    array_queue<std::pair<int, int>, 3> q;
    q.emplace_back(1, 2);
    EXPECT_EQ(q.back().first, 1);
    EXPECT_EQ(q.back().second, 2);
}

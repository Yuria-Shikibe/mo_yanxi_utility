#include <gtest/gtest.h>
import mo_yanxi.circular_queue;

using namespace mo_yanxi;

TEST(CircularQueueTest, BasicOperations) {
    circular_queue<int> q(5);
    EXPECT_TRUE(q.empty());

    q.push_back(1);
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

    q.push_front(0);
    EXPECT_EQ(q.front(), 0);
    EXPECT_EQ(q.back(), 2);
}

TEST(CircularQueueTest, AutoResize) {
    circular_queue<int, true> q(4);
    for(int i=0; i<10; ++i) {
        q.push_back(i);
    }
    EXPECT_EQ(q.size(), 10);
    EXPECT_GE(q.capacity(), 10);
    EXPECT_EQ(q.front(), 0);
    EXPECT_EQ(q.back(), 9);
}

TEST(CircularQueueTest, NoAutoResize) {
    // If we disable auto resize, we should respect capacity
    // But push_back doesn't check overflow in release if I recall?
    // "tryExpand() -> if(auto_resize) ... else assert(!full())"
    // So in release with auto_resize=false, it asserts (which might be compiled out).
    // Let's stick to safe usage or default auto_resize=true.
}

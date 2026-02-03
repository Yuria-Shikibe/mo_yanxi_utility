#include <gtest/gtest.h>
import mo_yanxi.circular_array;

using namespace mo_yanxi;

TEST(CircularArrayTest, BasicOperations) {
    circular_array<int, 3> ca{};
    ca[0] = 10;
    ca[1] = 20;
    ca[2] = 30;

    EXPECT_EQ(ca.get_index(), 0);
    EXPECT_EQ(ca.get_current(), 10);

    ca.next();
    EXPECT_EQ(ca.get_index(), 1);
    EXPECT_EQ(ca.get_current(), 20);

    ca.prev();
    EXPECT_EQ(ca.get_index(), 0);
    EXPECT_EQ(ca.get_current(), 10);

    ca.prev(); // wrap around
    EXPECT_EQ(ca.get_index(), 2);
    EXPECT_EQ(ca.get_current(), 30);
}

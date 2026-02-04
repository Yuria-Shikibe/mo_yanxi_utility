#include <gtest/gtest.h>
import mo_yanxi.flat_set;

using namespace mo_yanxi;

TEST(FlatSetTest, BasicOperations) {
    linear_flat_set<std::vector<int>> set;
    EXPECT_TRUE(set.empty());

    EXPECT_TRUE(set.insert(1));
    EXPECT_TRUE(set.contains(1));
    EXPECT_EQ(set.size(), 1);

    EXPECT_FALSE(set.insert(1));
    EXPECT_EQ(set.size(), 1);

    EXPECT_TRUE(set.insert(2));
    EXPECT_EQ(set.size(), 2);

    EXPECT_TRUE(set.erase(1));
    EXPECT_FALSE(set.contains(1));
    EXPECT_EQ(set.size(), 1);

    EXPECT_TRUE(set.update(2, 3));
    EXPECT_TRUE(set.contains(3));
    EXPECT_FALSE(set.contains(2));

    set.clear();
    EXPECT_TRUE(set.empty());
}

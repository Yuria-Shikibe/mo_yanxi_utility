#include <gtest/gtest.h>
import mo_yanxi.algo.hash;
#include <vector>

using namespace mo_yanxi;

TEST(HashTest, MakeHashAndAccess) {
    std::vector<int> table(10);
    std::vector<int> source = {1, 2, 3};

    algo::make_hash(table, source);

    auto it = algo::access_hash(table, 2);
    EXPECT_NE(it, table.end());
    EXPECT_EQ(*it, 2);

    it = algo::access_hash(table, 5);
    EXPECT_EQ(it, table.end());
}

#include <gtest/gtest.h>
import mo_yanxi.open_addr_hash_map;

using namespace mo_yanxi;

TEST(OpenHashMapTest, BasicOperations) {
    fixed_open_hash_map<int, std::string> map;
    EXPECT_TRUE(map.empty());

    map.insert({1, "one"});
    EXPECT_EQ(map.size(), 1);
    EXPECT_TRUE(map.contains(1));
    EXPECT_EQ(map.at(1), "one");

    map[2] = "two";
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[2], "two");

    map.erase(1);
    EXPECT_FALSE(map.contains(1));
    EXPECT_EQ(map.size(), 1);

    map.clear();
    EXPECT_TRUE(map.empty());
}

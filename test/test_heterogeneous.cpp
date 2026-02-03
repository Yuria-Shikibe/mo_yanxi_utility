#include <gtest/gtest.h>
import mo_yanxi.heterogeneous;
import mo_yanxi.heterogeneous.open_addr_hash;

using namespace mo_yanxi;

TEST(HeterogeneousTest, StringHashMap) {
    string_hash_map<int> map;
    map["one"] = 1;

    // String view lookup
    EXPECT_EQ(map.at("one"), 1);

    std::string_view sv = "one";
    EXPECT_EQ(map.at(sv), 1);

    map.insert_or_assign("two", 2);
    EXPECT_EQ(map.at("two"), 2);
}

TEST(HeterogeneousTest, StringOpenAddrHashMap) {
    string_open_addr_hash_map<int> map;
    map["one"] = 1;

    EXPECT_EQ(map.at("one"), 1);
    EXPECT_EQ(map["one"], 1);

    // Heterogeneous lookup
    EXPECT_TRUE(map.contains("one"));
    EXPECT_TRUE(map.contains(std::string_view("one")));
}

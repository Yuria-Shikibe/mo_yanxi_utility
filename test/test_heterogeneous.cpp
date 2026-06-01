#include <gtest/gtest.h>
import mo_yanxi.heterogeneous;

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

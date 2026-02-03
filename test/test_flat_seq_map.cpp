#include <gtest/gtest.h>
import mo_yanxi.flat_seq_map;

using namespace mo_yanxi;

TEST(FlatSeqMapTest, BasicOperations) {
    flat_seq_map<int, std::string> map;

    // Test insert_unique
    auto [it, inserted] = map.insert_unique(1, "one");
    EXPECT_TRUE(inserted);
    EXPECT_EQ(it->value, "one");

    auto [it2, inserted2] = map.insert_unique(1, "one_again");
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it2->value, "one");

    map.insert_unique(2, "two");

    // Test find
    auto found = map.find_unique(1);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, "one");

    auto found2 = map.find_unique(3);
    EXPECT_EQ(found2, nullptr);

    // Test each
    int count = 0;
    map.each([&](const int& k, std::string& v) {
        count++;
    });
    EXPECT_EQ(count, 2);
}

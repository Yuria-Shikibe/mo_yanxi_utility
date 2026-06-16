#include <gtest/gtest.h>
#include <string>
#include <string_view>
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

TEST(HeterogeneousTest, BasicStringHashMapWideString) {
    basic_string_hash_map<std::wstring, int> map;
    map[L"one"] = 1;

    std::wstring_view sv = L"one";
    EXPECT_EQ(map.at(sv), 1);
    ASSERT_NE(map.try_find(L"one"), nullptr);
    EXPECT_EQ(*map.try_find(L"one"), 1);

    map.try_emplace(std::wstring_view{L"two"}, 2);
    EXPECT_EQ(map.at(L"two"), 2);

    map.insert_or_assign(L"two", 3);
    EXPECT_EQ(map.at(std::wstring_view{L"two"}), 3);
}

TEST(HeterogeneousTest, BasicStringHashMapUtf8String) {
    basic_string_hash_map<std::u8string, int> map;
    map[u8"one"] = 1;

    std::u8string_view sv = u8"one";
    EXPECT_EQ(map.at(sv), 1);
    ASSERT_NE(map.try_find(u8"one"), nullptr);
    EXPECT_EQ(*map.try_find(u8"one"), 1);

    map.try_emplace(std::u8string_view{u8"two"}, 2);
    EXPECT_EQ(map.at(u8"two"), 2);

    map.insert_or_assign(u8"two", 3);
    EXPECT_EQ(map.at(std::u8string_view{u8"two"}), 3);
}

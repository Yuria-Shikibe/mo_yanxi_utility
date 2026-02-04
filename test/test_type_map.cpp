#include <gtest/gtest.h>
import mo_yanxi.type_map;

using namespace mo_yanxi;

TEST(TypeMapTest, BasicOperations) {
    using MyTypeMap = type_map<int, std::tuple<int, float, double>>;
    MyTypeMap tm;

    tm.at<int>() = 1;
    tm.at<float>() = 2;
    tm.at<double>() = 3;

    EXPECT_EQ(tm.at<int>(), 1);
    EXPECT_EQ(tm.at<float>(), 2);
    EXPECT_EQ(tm.at<double>(), 3);

    // Index access
    EXPECT_EQ(tm.at<0>(), 1);
}

#include <gtest/gtest.h>
import ext.fast_pop_stack;

using namespace mo_yanxi;

TEST(FastPopStackTest, BasicOperations) {
    fast_pop_stack<int, 10, 4> s;

    // push returns nullopt on success
    EXPECT_FALSE(s.push(1).has_value());
    EXPECT_FALSE(s.push(2).has_value());
    EXPECT_FALSE(s.push(3).has_value());

    int found_count = 0;
    for(int i=0; i<3; ++i) {
        auto val = s.pop();
        if (val.has_value()) {
            found_count++;
        }
    }
    EXPECT_EQ(found_count, 3);

    s.clear();
    EXPECT_FALSE(s.pop().has_value());
}

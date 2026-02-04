#include <gtest/gtest.h>
import mo_yanxi.history_stack;

using namespace mo_yanxi;

TEST(HistoryStackTest, BasicOperations) {
    history_stack<int> s;
    EXPECT_TRUE(s.empty());

    s.push(1);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.current(), 1);

    s.push(2);
    EXPECT_EQ(s.current(), 2);

    s.pop();
    EXPECT_EQ(s.current(), 1);
}

TEST(ProcedureHistoryStackTest, UndoRedo) {
    procedure_history_stack<int> s;
    s.push(1);
    s.push(2);

    // Initially at end (latest)
    EXPECT_TRUE(s.has_prev());
    EXPECT_FALSE(s.has_next());

    // Undo 2
    auto* val = s.to_prev();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);

    // Undo 1
    val = s.to_prev();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);

    // Redo 1
    val = s.to_next();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);

    // Redo 2
    val = s.to_next();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 2);

    EXPECT_FALSE(s.has_next());
}

TEST(ProcedureHistoryStackTest, Truncate) {
    procedure_history_stack<int> s;
    s.push(1);
    s.push(2);

    s.to_prev(); // Undo 2

    s.push(3); // Should truncate 2 and push 3

    EXPECT_FALSE(s.has_next());
    EXPECT_TRUE(s.has_prev());

    auto* val = s.to_prev();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 3);

    val = s.to_prev();
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, 1);
}

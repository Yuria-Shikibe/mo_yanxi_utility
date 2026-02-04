#include <gtest/gtest.h>
import mo_yanxi.byte_pool;

using namespace mo_yanxi;

TEST(BytePoolTest, BasicOperations) {
    byte_pool<std::allocator<std::byte>> pool;
    auto borrow = pool.borrow<int>(10);
    EXPECT_EQ(borrow.get().size(), 10);

    // Check if memory is writable
    borrow.get().data()[0] = 123;
    EXPECT_EQ(borrow.get().data()[0], 123);
}

TEST(BytePoolTest, Reuse) {
    byte_pool<> pool;
    {
        auto borrow = pool.borrow<int>(10);
    }
    // Memory should be returned to pool.
    auto borrow2 = pool.borrow<int>(10);
    EXPECT_EQ(borrow2.get().size(), 10);
}

TEST(BytePoolTest, Resize) {
    byte_pool<> pool;
    auto borrow = pool.borrow<int>(10);
    borrow.resize(20);
    EXPECT_EQ(borrow.get().size(), 20);

    borrow.resize(5);
    EXPECT_EQ(borrow.get().size(), 5);
}

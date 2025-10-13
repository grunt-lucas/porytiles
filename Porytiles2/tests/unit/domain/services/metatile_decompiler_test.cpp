#include "gtest/gtest.h"

#include "porytiles2/domain/services/metatile_decompiler.hpp"

using namespace porytiles2;

TEST(MetatileDecompilerTests, FooShouldBeZero)
{
    MetatileDecompiler foo{};
    EXPECT_EQ(foo.foo(), 0);
}

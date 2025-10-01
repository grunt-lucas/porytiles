#include "gtest/gtest.h"
    
#include "porytiles2/domain/services/assignable_tile_generator.hpp"

using namespace porytiles2;

TEST(AssignableTileGeneratorTests, FooShouldBeZero) {
    AssignableTileGenerator foo{};
    EXPECT_EQ(foo.foo(), 0);
}

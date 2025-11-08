#include "gtest/gtest.h"

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

TEST(PaletteTests, FooShouldBeZero)
{
    Palette<Rgba32> foo{};
}

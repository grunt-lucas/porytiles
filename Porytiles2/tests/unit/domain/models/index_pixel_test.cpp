#include "gtest/gtest.h"

#include "porytiles2/domain/models/index_pixel.hpp"

using namespace porytiles2;

TEST(IndexPixelTests, DefaultConstructedValueShouldBeTransparent)
{
    const IndexPixel default_pixel{};
    EXPECT_TRUE(default_pixel.is_transparent());
    EXPECT_EQ(default_pixel.index(), 0);
}

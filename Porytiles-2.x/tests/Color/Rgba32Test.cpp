#include <gtest/gtest.h>

#include "porytiles/Color/Rgba32.h"

TEST(Rgba32Test, TestJascString)
{
    using namespace porytiles::color;

    const Rgba32 transparentRgba{};
    EXPECT_EQ(transparentRgba.toJascString(), "0 0 0");

    const Rgba32 red{255, 0, 0};
    EXPECT_EQ(red.toJascString(), "255 0 0");

    const Rgba32 green{0, 255, 0, Rgba32::ALPHA_OPAQUE};
    EXPECT_EQ(green.toJascString(), "0 255 0");

    const Rgba32 magenta{255, 0, 255, 0};
    EXPECT_EQ(magenta.toJascString(), "255 0 255");
}

#include <gtest/gtest.h>

#include <porytiles/Color/Bgr15.h>
#include <porytiles/Color/Rgba32.h>
#include <porytiles/Color/RgbaToBgr.h>

TEST(RgbaToBgrTest, RgbaToBgr)
{
    using namespace porytiles::color;

    const RgbaToBgr rgbaToBgr{};

    constexpr Rgba32 redRgba1{255, 0, 0};
    constexpr Bgr15 redBgr1{31};
    const auto convertedBgr1 = rgbaToBgr.convert(redRgba1);
    EXPECT_EQ(convertedBgr1, redBgr1);

    constexpr Rgba32 redRgba2{250, 0, 0};
    const auto convertedBgr2 = rgbaToBgr.convert(redRgba2);
    // should still match redBgr1 due to precision loss between formats
    EXPECT_EQ(convertedBgr2, redBgr1);
}

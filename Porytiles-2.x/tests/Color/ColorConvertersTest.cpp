#include <gtest/gtest.h>

#include "porytiles/Color/Bgr15.h"
#include "porytiles/Color/ColorConverters.h"
#include "porytiles/Color/Rgba32.h"

TEST(ColorConvertersTest, RgbaToBgr)
{
    using namespace porytiles::color;

    constexpr Rgba32 redRgba1{255, 0, 0};
    constexpr Bgr15 redBgr1{31};
    constexpr Bgr15 redBgr2{255, 0, 0};
    EXPECT_EQ(rgbaToBgr(redRgba1), redBgr1);
    EXPECT_EQ(rgbaToBgr(redRgba1), redBgr2);

    constexpr Rgba32 yellowRgba1{255, 255, 0};
    constexpr Bgr15 yellowBgr1{1023};
    constexpr Bgr15 yellowBgr2{255, 255, 0};
    EXPECT_EQ(rgbaToBgr(yellowRgba1), yellowBgr1);
    EXPECT_EQ(rgbaToBgr(yellowRgba1), yellowBgr2);
}

TEST(ColorConvertersTest, BgrToRgba)
{
    using namespace porytiles::color;

    constexpr Bgr15 blueBgr1{0, 0, 255};
    constexpr Rgba32 blueRgba1{0, 0, 248};

    EXPECT_EQ(bgrToRgba(blueBgr1), blueRgba1);
}

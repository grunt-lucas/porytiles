#include <gtest/gtest.h>

#include <porytiles/Color/Bgr15.h>
#include <porytiles/Color/Rgba32.h>
#include <porytiles/Color/Palette.h>
#include <porytiles/Color/RgbaToBgr.h>

TEST(PaletteTest, Foo)
{
    using namespace porytiles::color;

    Palette<Bgr15> palette{10};
    RgbaToBgr rgbaToBgr{};
    palette.foo(rgbaToBgr, RGBA_RED);
}

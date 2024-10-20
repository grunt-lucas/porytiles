#include <gtest/gtest.h>

#include "porytiles/Color/Bgr15.h"
#include "porytiles/Color/Rgba32.h"

TEST(Bgr15Test, TestDefaultCtor) {
    using namespace porytiles::color;
    const Bgr15 bgr{};
    EXPECT_EQ(bgr.getRawValue(), 0);
}

TEST(Rgba32Test, TestJascString) {
    using namespace porytiles::color;
    const Rgba32 rgb{};
    EXPECT_EQ(rgb.toJascString(), "0 0 0");
}

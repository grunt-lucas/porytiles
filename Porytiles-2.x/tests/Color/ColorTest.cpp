#include <gtest/gtest.h>

#include "porytiles/Color/Bgr15.h"

TEST(Bgr15Test, TestDefaultCtor) {
    using namespace porytiles::color;
    const Bgr15 bgr{};
    EXPECT_EQ(bgr.getRawValue(), 0);
}

#include <gtest/gtest.h>

#include "porytiles/Color/Rgba32.h"

TEST(Rgba32Test, TestJascString) {
    using namespace porytiles::color;
    const Rgba32 rgb{};
    EXPECT_EQ(rgb.toJascString(), "0 0 0");
}

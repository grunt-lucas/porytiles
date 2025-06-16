#include <gtest/gtest.h>

#include "png/impl/cimg_png.hpp"

using namespace porytiles;

TEST(CImgPngTests, DimensionsMethodsShouldWork) {
    CImgPng png{};

    EXPECT_EQ(png.Width(), 0);
    EXPECT_EQ(png.Height(), 0);

    const auto result = png.Read("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(png.Width(), 16);
    EXPECT_EQ(png.Height(), 16);
}

TEST(CImgPngTests, OpenShouldFailGracefullyOnBadFile) {
    CImgPng png{};

    const auto result = png.Read("Resources/Tests/Unit/png/non_existent_file.png");
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().contains("Failed to open file 'Resources/Tests/Unit/png/non_existent_file.png'"));

    const auto result2 = png.Read("Resources/Tests/Unit/metatile_behaviors.h");
    ASSERT_FALSE(result2.has_value());
    EXPECT_TRUE(
        result2.error().contains("Failed to recognize format of file 'Resources/Tests/Unit/metatile_behaviors.h'"));
}

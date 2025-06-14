#include <gtest/gtest.h>

#include <porytiles2/png/cimg_png.hpp>

using namespace porytiles;

TEST(CImgPngTests, DimensionsMethodsShouldWork) {
    CImgPng png{};
    ASSERT_EQ(png.Width(), 0);
    ASSERT_EQ(png.Height(), 0);

    const auto result = png.Read("Resources/Tests/png/pattern.png");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(png.Width(), 16);
    ASSERT_EQ(png.Height(), 16);
}

TEST(CImgPngTests, OpenShouldFailGracefullyOnBadFile) {
    CImgPng png{};

    const auto result = png.Read("Resources/Tests/png/non_existent_file.png");
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.error().contains("Failed to open file 'Resources/Tests/png/non_existent_file.png'"));

    const auto result2 = png.Read("Resources/Tests/metatile_behaviors.h");
    ASSERT_FALSE(result2.has_value());
    ASSERT_TRUE(result2.error().contains("Failed to recognize format of file 'Resources/Tests/metatile_behaviors.h'"));
}

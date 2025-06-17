#include "png/impl/cimg_png_importer.hpp"

#include <gtest/gtest.h>

#include <png/impl/cimg_png.hpp>
#include <porytiles2/png/png_importer.hpp>

using namespace porytiles;

TEST(CImgPngTests, DimensionsMethodsShouldWork) {
    const std::unique_ptr<PngImporter> importer = std::make_unique<CImgPngImporter>();
    auto result = importer->Read("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());

    const auto png = std::move(result.value());
    EXPECT_EQ(png->Width(), 16);
    EXPECT_EQ(png->Height(), 16);
}

TEST(CImgPngTests, OpenShouldFailGracefullyOnBadFile) {
    const std::unique_ptr<PngImporter> importer = std::make_unique<CImgPngImporter>();

    auto result = importer->Read("Resources/Tests/Unit/png/non_existent_file.png");
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().contains("Failed to open file 'Resources/Tests/Unit/png/non_existent_file.png'"));

    auto result2 = importer->Read("Resources/Tests/Unit/metatile_behaviors.h");
    ASSERT_FALSE(result2.has_value());
    EXPECT_TRUE(
        result2.error().contains("Failed to recognize format of file 'Resources/Tests/Unit/metatile_behaviors.h'"));
}

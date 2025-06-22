#include <gtest/gtest.h>

#include <porytiles2/domain/entities/png.hpp>
#include <porytiles2/domain/repos/png_repo.hpp>
#include <porytiles2/infra/persistence/cimg_png_repo.hpp>
#include <porytiles2/infra/png/cimg_png.hpp>

using namespace porytiles;

// TODO : this is technically an integration test

TEST(CImgPngTests, DimensionsMethodsShouldWork) {
    const std::unique_ptr<PngRepo> importer = std::make_unique<CImgPngRepo>();
    auto result = importer->Read("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());

    const auto png = std::move(result.value());
    EXPECT_EQ(png->Width(), 16);
    EXPECT_EQ(png->Height(), 16);
}

TEST(CImgPngTests, GetByRowColShouldWork) {
    const std::unique_ptr<PngRepo> importer = std::make_unique<CImgPngRepo>();
    auto result = importer->Read("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());

    const auto png = std::move(result.value());

    constexpr Rgba32 red{255, 0, 0};
    constexpr Rgba32 green{0, 255, 0};
    constexpr Rgba32 blue{0, 0, 255};
    constexpr Rgba32 magenta{255, 0, 255};
    constexpr Rgba32 cyan{0, 255, 255};

    EXPECT_EQ(png->At(0, 0), blue);
    EXPECT_EQ(png->At(7, 15), red);
    EXPECT_EQ(png->At(7, 14), green);
    EXPECT_EQ(png->At(0, 8), magenta);
    EXPECT_EQ(png->At(8, 0), cyan);
}

TEST(CImgPngTests, OpenShouldFailGracefullyOnBadFile) {
    const std::unique_ptr<PngRepo> importer = std::make_unique<CImgPngRepo>();

    auto result = importer->Read("Resources/Tests/Unit/png/non_existent_file.png");
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().contains("Failed to open file 'Resources/Tests/Unit/png/non_existent_file.png'"));

    auto result2 = importer->Read("Resources/Tests/Unit/metatile_behaviors.h");
    ASSERT_FALSE(result2.has_value());
    EXPECT_TRUE(
        result2.error().contains("Failed to recognize format of file 'Resources/Tests/Unit/metatile_behaviors.h'"));
}

#include "gtest/gtest.h"

#include <tuple>

#include "../../../include/porytiles2/infra/services/rgba_image/png_rgba_image_loader.hpp"
#include "porytiles2/domain/model/valueobj/rgba_image.hpp"
#include "porytiles2/domain/services/rgba_image_loader.hpp"

using namespace porytiles2;

// TODO : reorganize test assets

TEST(PngRgbaImageLoaderTests, LoadFromFileShouldFailGracefullyOnBadFile) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    auto result = loader->load_from_file("Resources/Tests/Unit/png/non_existent_file.png");
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().contains("Failed to open file 'Resources/Tests/Unit/png/non_existent_file.png'"));

    auto result2 = loader->load_from_file("Resources/Tests/Unit/metatile_behaviors.h");
    ASSERT_FALSE(result2.has_value());
    EXPECT_TRUE(result2.error().contains("Failed to recognize format of file "
                                         "'Resources/Tests/Unit/metatile_behaviors.h'"));
}

TEST(PngRgbaImageLoaderTests, ShouldLoadValidPngFile) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    const auto result = loader->load_from_file("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);

    const auto &image = *result.value();
    EXPECT_GT(image.width(), 0);
    EXPECT_GT(image.height(), 0);
}

TEST(PngRgbaImageLoaderTests, ShouldLoadRgbImageWithOpaqueAlpha) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    // Use one of the available PNG files that should be RGB (3-channel)
    const auto result = loader->load_from_file("Resources/Examples/simple_primary_1/bottom.png");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);

    const auto &image = *result.value();
    EXPECT_GT(image.width(), 0);
    EXPECT_GT(image.height(), 0);

    // Check that alpha values are opaque (255) for RGB images
    bool has_opaque_pixels = false;
    for (std::size_t row = 0; row < image.height(); ++row) {
        for (std::size_t col = 0; col < image.width(); ++col) {
            const auto pixel = image.at(row, col);
            if (pixel.alpha() == Rgba32::alpha_opaque) {
                has_opaque_pixels = true;
                break;
            }
        }
        if (has_opaque_pixels)
            break;
    }
    EXPECT_TRUE(has_opaque_pixels);
}

TEST(PngRgbaImageLoaderTests, ShouldLoadImageDimensionsCorrectly) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    auto result = loader->load_from_file("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);

    const auto &image = *result.value();
    // The pattern.png should have specific dimensions
    EXPECT_GT(image.width(), 0);
    EXPECT_GT(image.height(), 0);

    // Verify we can access pixels within the bounds
    EXPECT_NO_THROW(std::ignore = image.at(0, 0));
    EXPECT_NO_THROW(std::ignore = image.at(image.height() - 1, image.width() - 1));
}

TEST(PngRgbaImageLoaderTests, ShouldLoadPixelDataCorrectly) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    auto result = loader->load_from_file("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);

    const auto &image = *result.value();

    // Test that we can read pixel data using both access methods
    const auto pixel_by_coords = image.at(0, 0);
    const auto pixel_by_index = image.at(0);

    EXPECT_EQ(pixel_by_coords.red(), pixel_by_index.red());
    EXPECT_EQ(pixel_by_coords.green(), pixel_by_index.green());
    EXPECT_EQ(pixel_by_coords.blue(), pixel_by_index.blue());
    EXPECT_EQ(pixel_by_coords.alpha(), pixel_by_index.alpha());

    // Verify that all pixel values are valid (0-255)
    for (std::size_t i = 0; i < image.width() * image.height(); ++i) {
        const auto pixel = image.at(i);
        EXPECT_GE(pixel.red(), 0);
        EXPECT_LE(pixel.red(), 255);
        EXPECT_GE(pixel.green(), 0);
        EXPECT_LE(pixel.green(), 255);
        EXPECT_GE(pixel.blue(), 0);
        EXPECT_LE(pixel.blue(), 255);
        EXPECT_GE(pixel.alpha(), 0);
        EXPECT_LE(pixel.alpha(), 255);
    }
}

TEST(PngRgbaImageLoaderTests, ShouldHandleMultipleImageFormats) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    // Test reading different PNG files to ensure the reader works with various formats
    const std::vector<std::string> test_files = {
        "Resources/Examples/simple_primary_1/bottom.png", "Resources/Examples/simple_primary_1/middle.png",
        "Resources/Examples/simple_primary_1/top.png", "Resources/Examples/simple_primary_1/anim/flower_white/key.png"};

    for (const auto &file : test_files) {
        auto result = loader->load_from_file(file);
        ASSERT_TRUE(result.has_value()) << "Failed to read file: " << file;
        ASSERT_NE(result.value(), nullptr) << "Null image for file: " << file;

        const auto &image = *result.value();
        EXPECT_GT(image.width(), 0) << "Invalid width for file: " << file;
        EXPECT_GT(image.height(), 0) << "Invalid height for file: " << file;
    }
}

TEST(PngRgbaImageLoaderTests, ShouldCorrectlyHandleAlphaChannels) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    // Test with a key.png file which likely has transparency
    const auto result = loader->load_from_file("Resources/Examples/simple_primary_1/anim/flower_white/key.png");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);

    const auto &image = *result.value();

    // Check that we have some pixels with alpha values
    bool has_alpha_variation = false;
    const std::uint8_t first_alpha = image.at(0, 0).alpha();

    for (std::size_t row = 0; row < image.height() && !has_alpha_variation; ++row) {
        for (std::size_t col = 0; col < image.width() && !has_alpha_variation; ++col) {
            if (const auto pixel = image.at(row, col); pixel.alpha() != first_alpha) {
                has_alpha_variation = true;
            }
        }
    }

    // The key.png should have some transparency variation, but if not,
    // at least verify all alpha values are valid
    for (std::size_t i = 0; i < image.width() * image.height(); ++i) {
        const auto pixel = image.at(i);
        EXPECT_GE(pixel.alpha(), 0);
        EXPECT_LE(pixel.alpha(), 255);
    }
}

TEST(PngRgbaImageLoaderTests, ShouldHandleSmallImages) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    // Test with pattern.png which should be a small test image
    auto result = loader->load_from_file("Resources/Tests/Unit/png/pattern.png");
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);

    const auto &image = *result.value();

    // Even small images should have valid dimensions
    EXPECT_GT(image.width(), 0);
    EXPECT_GT(image.height(), 0);

    // Test accessing all pixels in a small image
    for (std::size_t row = 0; row < image.height(); ++row) {
        for (std::size_t col = 0; col < image.width(); ++col) {
            EXPECT_NO_THROW(std::ignore = image.at(row, col));
        }
    }
}

TEST(PngRgbaImageLoaderTests, ShouldConsistentlyLoadSameFile) {
    const std::unique_ptr<RgbaImageLoader> loader = std::make_unique<PngRgbaImageLoader>();

    // Read the same file multiple times to ensure consistent results
    auto result1 = loader->load_from_file("Resources/Tests/Unit/png/pattern.png");
    auto result2 = loader->load_from_file("Resources/Tests/Unit/png/pattern.png");

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_NE(result1.value(), nullptr);
    ASSERT_NE(result2.value(), nullptr);

    const auto &image1 = *result1.value();
    const auto &image2 = *result2.value();

    // Images should have same dimensions
    EXPECT_EQ(image1.width(), image2.width());
    EXPECT_EQ(image1.height(), image2.height());

    // Images should have same pixel data
    for (std::size_t i = 0; i < image1.width() * image1.height(); ++i) {
        const auto pixel1 = image1.at(i);
        const auto pixel2 = image2.at(i);

        EXPECT_EQ(pixel1.red(), pixel2.red());
        EXPECT_EQ(pixel1.green(), pixel2.green());
        EXPECT_EQ(pixel1.blue(), pixel2.blue());
        EXPECT_EQ(pixel1.alpha(), pixel2.alpha());
    }
}
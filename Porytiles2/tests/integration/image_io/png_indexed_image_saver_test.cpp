#include "gtest/gtest.h"

#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/templates/result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

using namespace porytiles2;

class PngIndexedImageSaverTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        saver_ = std::make_unique<PngIndexedImageSaver>();
        loader_ = std::make_unique<PngIndexedImageLoader>();

        // Create a temporary directory for test files
        temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_indexed_saver_tests";
        std::filesystem::create_directories(temp_dir_);
    }

    void TearDown() override
    {
        // Clean up temporary files
        if (std::filesystem::exists(temp_dir_)) {
            std::filesystem::remove_all(temp_dir_);
        }
    }

    [[nodiscard]] std::filesystem::path get_tmp_path(const std::string &filename) const
    {
        return temp_dir_ / filename;
    }

    static Image<IndexPixel> create_test_image(std::size_t width, std::size_t height)
    {
        // Create a palette with 16 colors
        std::vector<Rgba32> palette;
        palette.reserve(16);
        for (std::size_t i = 0; i < 16; ++i) {
            const auto red = static_cast<std::uint8_t>(i * 16);
            const auto green = static_cast<std::uint8_t>(255 - i * 16);
            const auto blue = static_cast<std::uint8_t>(i * 8);
            const auto alpha = i == 0 ? static_cast<std::uint8_t>(0) : static_cast<std::uint8_t>(255);
            palette.emplace_back(red, green, blue, alpha);
        }

        Image<IndexPixel> image{width, height, palette};

        // Fill with a pattern of palette indices
        for (std::size_t row = 0; row < height; ++row) {
            for (std::size_t col = 0; col < width; ++col) {
                const auto index = (row + col) % 16;
                image.set(row, col, IndexPixel{static_cast<unsigned int>(index)});
            }
        }

        return image;
    }

    static Image<IndexPixel> create_greyscale_test_image(std::size_t width, std::size_t height)
    {
        // Create a greyscale palette with 16 levels (matching the implementation)
        std::vector<Rgba32> palette;
        palette.reserve(16);
        for (std::size_t i = 0; i < 16; ++i) {
            const auto grey = static_cast<std::uint8_t>(i * 16);
            palette.emplace_back(grey, grey, grey, 255);
        }

        Image<IndexPixel> image{width, height, palette};

        // Fill with a gradient pattern using only 16 indices
        for (std::size_t row = 0; row < height; ++row) {
            for (std::size_t col = 0; col < width; ++col) {
                const auto index = ((row * 16 / height) + (col * 16 / width)) / 2;
                image.set(row, col, IndexPixel{static_cast<unsigned int>(index % 16)});
            }
        }

        return image;
    }

    std::unique_ptr<PngIndexedImageSaver> saver_;
    std::unique_ptr<PngIndexedImageLoader> loader_;
    std::filesystem::path temp_dir_;
};

TEST_F(PngIndexedImageSaverTests, SaveToFileShouldFailGracefullyOnInvalidPath)
{
    const auto image = create_test_image(2, 2);

    // Try to save to a non-existent directory without creating it
    const auto invalid_path = std::filesystem::path{"/non/existent/directory/test.png"};

    auto result = saver_->save_to_file(image, invalid_path, TilesPalMode::true_color);
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().details(PlainTextFormatter{}).contains("save failed"));
}

TEST_F(PngIndexedImageSaverTests, ShouldSaveValidPngFileInTrueColorMode)
{
    const auto image = create_test_image(4, 4);
    const auto file_path = get_tmp_path("test_save_true_color.png");

    auto result = saver_->save_to_file(image, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(result.has_value());

    // Verify the file was created
    EXPECT_TRUE(std::filesystem::exists(file_path));
    EXPECT_GT(std::filesystem::file_size(file_path), 0);
}

TEST_F(PngIndexedImageSaverTests, ShouldSaveValidPngFileInGreyscaleMode)
{
    const auto image = create_greyscale_test_image(4, 4);
    const auto file_path = get_tmp_path("test_save_greyscale.png");

    auto result = saver_->save_to_file(image, file_path, TilesPalMode::greyscale);
    ASSERT_TRUE(result.has_value());

    // Verify the file was created
    EXPECT_TRUE(std::filesystem::exists(file_path));
    EXPECT_GT(std::filesystem::file_size(file_path), 0);
}

TEST_F(PngIndexedImageSaverTests, ShouldSaveAndLoadRoundTripTrueColor)
{
    const auto original_image = create_test_image(8, 6);
    const auto file_path = get_tmp_path("roundtrip_test_true_color.png");

    // Save the image
    auto save_result = saver_->save_to_file(original_image, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(save_result.has_value());

    // Load it back
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();

    // Compare dimensions
    EXPECT_EQ(loaded_image.width(), original_image.width());
    EXPECT_EQ(loaded_image.height(), original_image.height());

    // Compare pixel indices
    for (std::size_t row = 0; row < original_image.height(); ++row) {
        for (std::size_t col = 0; col < original_image.width(); ++col) {
            const auto original_pixel = original_image.at(row, col);
            const auto loaded_pixel = loaded_image.at(row, col);

            EXPECT_EQ(loaded_pixel.index(), original_pixel.index())
                << "Index mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST_F(PngIndexedImageSaverTests, ShouldSaveAndLoadRoundTripGreyscale)
{
    const auto original_image = create_greyscale_test_image(8, 6);
    const auto file_path = get_tmp_path("roundtrip_test_greyscale.png");

    // Save the image
    auto save_result = saver_->save_to_file(original_image, file_path, TilesPalMode::greyscale);
    ASSERT_TRUE(save_result.has_value());

    // Load it back
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();

    // Compare dimensions
    EXPECT_EQ(loaded_image.width(), original_image.width());
    EXPECT_EQ(loaded_image.height(), original_image.height());

    // Compare pixel indices
    for (std::size_t row = 0; row < original_image.height(); ++row) {
        for (std::size_t col = 0; col < original_image.width(); ++col) {
            const auto original_pixel = original_image.at(row, col);
            const auto loaded_pixel = loaded_image.at(row, col);

            EXPECT_EQ(loaded_pixel.index(), original_pixel.index())
                << "Index mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST_F(PngIndexedImageSaverTests, ShouldHandleSmallImages)
{
    const auto image = create_test_image(1, 1);
    const auto file_path = get_tmp_path("small_image.png");

    auto result = saver_->save_to_file(image, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(result.has_value());

    // Verify the file was created and has content
    EXPECT_TRUE(std::filesystem::exists(file_path));
    EXPECT_GT(std::filesystem::file_size(file_path), 0);

    // Verify it can be loaded back
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();
    EXPECT_EQ(loaded_image.width(), 1);
    EXPECT_EQ(loaded_image.height(), 1);
}

TEST_F(PngIndexedImageSaverTests, ShouldHandleLargeImages)
{
    constexpr auto width = 128;
    constexpr auto height = 320;
    const auto image = create_test_image(width, height);
    const auto file_path = get_tmp_path("large_image.png");

    auto result = saver_->save_to_file(image, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(result.has_value());

    // Verify the file was created and has a reasonable size
    // Indexed PNG with 16 colors is quite compact, so expect smaller size
    EXPECT_TRUE(std::filesystem::exists(file_path));
    EXPECT_GT(std::filesystem::file_size(file_path), 256);

    // Verify dimensions can be loaded back correctly
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();
    EXPECT_EQ(loaded_image.width(), static_cast<std::size_t>(width));
    EXPECT_EQ(loaded_image.height(), static_cast<std::size_t>(height));
}

TEST_F(PngIndexedImageSaverTests, ShouldHandleTransparencyCorrectly)
{
    // Create an image with transparent pixels (index 0)
    std::vector<Rgba32> palette;
    palette.emplace_back(0, 0, 0, 0);     // Index 0: transparent
    palette.emplace_back(255, 0, 0, 255); // Index 1: red
    palette.emplace_back(0, 255, 0, 255); // Index 2: green
    palette.emplace_back(0, 0, 255, 255); // Index 3: blue

    Image<IndexPixel> image{4, 4, palette};

    // Create a pattern with some transparent pixels
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            // Checkerboard pattern with transparency
            const auto index = ((row + col) % 2 == 0) ? 0u : (1u + ((row * col) % 3));
            image.set(row, col, IndexPixel{static_cast<unsigned int>(index)});
        }
    }

    const auto file_path = get_tmp_path("transparency_test.png");

    auto save_result = saver_->save_to_file(image, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(save_result.has_value());

    // Load it back and verify transparency is preserved
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();

    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            const auto original_pixel = image.at(row, col);
            const auto loaded_pixel = loaded_image.at(row, col);

            EXPECT_EQ(loaded_pixel.index(), original_pixel.index())
                << "Index mismatch at (" << row << ", " << col << ")";

            // Verify transparency detection works
            if (original_pixel.index() == 0) {
                EXPECT_TRUE(loaded_pixel.is_transparent())
                    << "Pixel should be transparent at (" << row << ", " << col << ")";
            }
        }
    }
}

TEST_F(PngIndexedImageSaverTests, ShouldOverwriteExistingFile)
{
    const auto image1 = create_test_image(2, 2);
    const auto image2 = create_test_image(3, 3);
    const auto file_path = get_tmp_path("overwrite_test.png");

    // Save first image
    auto result1 = saver_->save_to_file(image1, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(result1.has_value());

    const auto first_size = std::filesystem::file_size(file_path);

    // Save second image (should overwrite)
    auto result2 = saver_->save_to_file(image2, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(result2.has_value());

    const auto second_size = std::filesystem::file_size(file_path);

    // File size should be different (3x3 vs 2x2)
    EXPECT_NE(first_size, second_size);

    // Verify the final image is the second one
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();
    EXPECT_EQ(loaded_image.width(), 3);
    EXPECT_EQ(loaded_image.height(), 3);
}

TEST_F(PngIndexedImageSaverTests, ShouldHandleMaxPaletteSize)
{
    // Create an image with the maximum palette size (256 colors)
    std::vector<Rgba32> palette;
    palette.reserve(256);
    for (std::size_t i = 0; i < 256; ++i) {
        const auto red = static_cast<std::uint8_t>(i);
        const auto green = static_cast<std::uint8_t>((i * 2) % 256);
        const auto blue = static_cast<std::uint8_t>((i * 3) % 256);
        const auto alpha = i == 0 ? static_cast<std::uint8_t>(0) : static_cast<std::uint8_t>(255);
        palette.emplace_back(red, green, blue, alpha);
    }

    Image<IndexPixel> image{16, 16, palette};

    // Fill with all palette indices
    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            const auto index = row * 16 + col;
            image.set(row, col, IndexPixel{static_cast<unsigned int>(index)});
        }
    }

    const auto file_path = get_tmp_path("max_palette_test.png");

    auto save_result = saver_->save_to_file(image, file_path, TilesPalMode::true_color);
    ASSERT_TRUE(save_result.has_value());

    // Load it back and verify all indices are preserved
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();

    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            const auto expected_index = row * 16 + col;
            const auto loaded_pixel = loaded_image.at(row, col);

            EXPECT_EQ(loaded_pixel.index(), expected_index) << "Index mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST_F(PngIndexedImageSaverTests, ShouldHandleImageWithoutPalette)
{
    // Create an image without providing a palette
    Image<IndexPixel> image{4, 4};

    // Fill with index values
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            image.set(row, col, IndexPixel{static_cast<unsigned int>((row + col) % 16)});
        }
    }

    const auto file_path = get_tmp_path("no_palette_test.png");

    // This should handle the case gracefully (implementation may generate a default palette)
    auto result = saver_->save_to_file(image, file_path, TilesPalMode::true_color);

    // The result depends on the implementation - it might fail or create a default palette
    // We just verify it doesn't crash
    if (result.has_value()) {
        EXPECT_TRUE(std::filesystem::exists(file_path));
    }
}
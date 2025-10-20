#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <random>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/templates/result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

using namespace porytiles2;

class PngRgbaImageSaverTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        saver_ = std::make_unique<PngRgbaImageSaver>();
        loader_ = std::make_unique<PngRgbaImageLoader>();

        // Create a temporary directory for test files
        temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_saver_tests";
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

    static Image<Rgba32> create_test_image(std::size_t width, std::size_t height)
    {
        Image<Rgba32> image{width, height};

        // Fill with a simple pattern for testing
        for (std::size_t row = 0; row < height; ++row) {
            for (std::size_t col = 0; col < width; ++col) {
                const auto red = static_cast<std::uint8_t>((row * 255) / height);
                const auto green = static_cast<std::uint8_t>((col * 255) / width);
                const auto blue = static_cast<std::uint8_t>((row + col) % 256);
                const auto alpha = static_cast<std::uint8_t>(255 - ((row + col) % 256));

                image.set(row, col, Rgba32{red, green, blue, alpha});
            }
        }

        return image;
    }

    std::unique_ptr<PngRgbaImageSaver> saver_;
    std::unique_ptr<PngRgbaImageLoader> loader_;
    std::filesystem::path temp_dir_;
};

TEST_F(PngRgbaImageSaverTests, SaveToFileShouldFailGracefullyOnInvalidPath)
{
    const auto image = create_test_image(2, 2);

    // Try to save to a non-existent directory without creating it
    const auto invalid_path = std::filesystem::path{"/non/existent/directory/test.png"};

    auto result = saver_->save_to_file(image, invalid_path);
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().details(PlainTextFormatter{}).contains("save failed"));
}

TEST_F(PngRgbaImageSaverTests, ShouldSaveValidPngFile)
{
    const auto image = create_test_image(4, 4);
    const auto file_path = get_tmp_path("test_save.png");

    auto result = saver_->save_to_file(image, file_path);
    ASSERT_TRUE(result.has_value());

    // Verify the file was created
    EXPECT_TRUE(std::filesystem::exists(file_path));
    EXPECT_GT(std::filesystem::file_size(file_path), 0);
}

TEST_F(PngRgbaImageSaverTests, ShouldSaveAndLoadRoundTrip)
{
    const auto original_image = create_test_image(8, 6);
    const auto file_path = get_tmp_path("roundtrip_test.png");

    // Save the image
    auto save_result = saver_->save_to_file(original_image, file_path);
    ASSERT_TRUE(save_result.has_value());

    // Load it back
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();

    // Compare dimensions
    EXPECT_EQ(loaded_image.width(), original_image.width());
    EXPECT_EQ(loaded_image.height(), original_image.height());

    // Compare pixel data
    for (std::size_t row = 0; row < original_image.height(); ++row) {
        for (std::size_t col = 0; col < original_image.width(); ++col) {
            const auto original_pixel = original_image.at(row, col);
            const auto loaded_pixel = loaded_image.at(row, col);

            EXPECT_EQ(loaded_pixel.red(), original_pixel.red()) << "Red mismatch at (" << row << ", " << col << ")";
            EXPECT_EQ(loaded_pixel.green(), original_pixel.green())
                << "Green mismatch at (" << row << ", " << col << ")";
            EXPECT_EQ(loaded_pixel.blue(), original_pixel.blue()) << "Blue mismatch at (" << row << ", " << col << ")";
            EXPECT_EQ(loaded_pixel.alpha(), original_pixel.alpha())
                << "Alpha mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST_F(PngRgbaImageSaverTests, ShouldHandleSmallImages)
{
    const auto image = create_test_image(1, 1);
    const auto file_path = get_tmp_path("small_image.png");

    auto result = saver_->save_to_file(image, file_path);
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

TEST_F(PngRgbaImageSaverTests, ShouldHandleLargeImages)
{
    const auto width = 128;
    const auto height = 320;
    const auto image = create_test_image(width, height);
    const auto file_path = get_tmp_path("large_image.png");

    auto result = saver_->save_to_file(image, file_path);
    ASSERT_TRUE(result.has_value());

    // Verify the file was created and has a reasonable size
    // PNG compression is quite good, should be at least a half KiB
    EXPECT_TRUE(std::filesystem::exists(file_path));
    EXPECT_GT(std::filesystem::file_size(file_path), 512);

    // Verify dimensions can be loaded back correctly
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();
    EXPECT_EQ(loaded_image.width(), width);
    EXPECT_EQ(loaded_image.height(), height);
}

TEST_F(PngRgbaImageSaverTests, ShouldHandleTransparencyCorrectly)
{
    Image<Rgba32> image{4, 4};

    // Create a pattern with varying transparency
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t col = 0; col < 4; ++col) {
            const auto alpha = static_cast<std::uint8_t>((row * col * 64) % 256);
            image.set(row, col, Rgba32{255, 128, 64, alpha});
        }
    }

    const auto file_path = get_tmp_path("transparency_test.png");

    auto save_result = saver_->save_to_file(image, file_path);
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

            EXPECT_EQ(loaded_pixel.alpha(), original_pixel.alpha())
                << "Alpha mismatch at (" << row << ", " << col << ")";
        }
    }
}

TEST_F(PngRgbaImageSaverTests, ShouldOverwriteExistingFile)
{
    const auto image1 = create_test_image(2, 2);
    const auto image2 = create_test_image(3, 3);
    const auto file_path = get_tmp_path("overwrite_test.png");

    // Save first image
    auto result1 = saver_->save_to_file(image1, file_path);
    ASSERT_TRUE(result1.has_value());

    const auto first_size = std::filesystem::file_size(file_path);

    // Save second image (should overwrite)
    auto result2 = saver_->save_to_file(image2, file_path);
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

TEST_F(PngRgbaImageSaverTests, ShouldHandleOpaqueImages)
{
    Image<Rgba32> image{3, 3};

    // Create a fully opaque image
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t col = 0; col < 3; ++col) {
            image.set(row, col, Rgba32{255, 0, 0, 255}); // Red with full opacity
        }
    }

    const auto file_path = get_tmp_path("opaque_test.png");

    auto save_result = saver_->save_to_file(image, file_path);
    ASSERT_TRUE(save_result.has_value());

    // Load it back and verify all pixels are opaque
    auto load_result = loader_->load_from_file(file_path);
    ASSERT_TRUE(load_result.has_value());
    ASSERT_NE(load_result.value(), nullptr);

    const auto &loaded_image = *load_result.value();

    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t col = 0; col < 3; ++col) {
            const auto pixel = loaded_image.at(row, col);
            EXPECT_EQ(pixel.alpha(), 255) << "Pixel should be opaque at (" << row << ", " << col << ")";
            EXPECT_EQ(pixel.red(), 255) << "Red should be 255 at (" << row << ", " << col << ")";
            EXPECT_EQ(pixel.green(), 0) << "Green should be 0 at (" << row << ", " << col << ")";
            EXPECT_EQ(pixel.blue(), 0) << "Blue should be 0 at (" << row << ", " << col << ")";
        }
    }
}
#include "gtest/gtest.h"

#include <memory>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tile/rgba_metatile.hpp"
#include "porytiles2/domain/models/tile/tile_constants.hpp"
#include "porytiles2/domain/services/rgba_layer_image_metatileizer.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

using namespace porytiles2;

class RgbaLayerImageMetatileizerTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        loader_ = std::make_unique<PngRgbaImageLoader>();
        metatileizer_ = std::make_unique<RgbaLayerImageMetatileizer>();
    }

    std::unique_ptr<PngRgbaImageLoader> loader_;
    std::unique_ptr<RgbaLayerImageMetatileizer> metatileizer_;
};

TEST_F(RgbaLayerImageMetatileizerTests, ShouldSuccessfullyConstructMetatilesFromValidImages)
{
    // Load test images
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/services/bottom1.png");
    auto middle_result = loader_->load_from_file("Resources/Tests/unit/domain/services/middle1.png");
    auto top_result = loader_->load_from_file("Resources/Tests/unit/domain/services/top1.png");

    ASSERT_TRUE(bottom_result.has_value()) << "Failed to load bottom test image";
    ASSERT_TRUE(middle_result.has_value()) << "Failed to load middle test image";
    ASSERT_TRUE(top_result.has_value()) << "Failed to load top test image";

    // Execute metatileization
    auto result = metatileizer_->metatileize(*bottom_result.value(), *middle_result.value(), *top_result.value());

    // Verify success
    ASSERT_TRUE(result.has_value()) << "Metatileization failed: " << result.error().details(PlainTextFormatter{});

    const auto &rgba_metatiles = result.value();

    // Verify expected number of metatiles (128x128 image = 8x8 metatiles = 64 total)
    EXPECT_EQ(rgba_metatiles.size(), 64);
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldFailWithMismatchedImageDimensions)
{
    // Load test images
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/services/bottom1.png");
    ASSERT_TRUE(bottom_result.has_value());

    // Create images with different dimensions
    Image<Rgba32> smaller_image{64, 64}; // Different from 128x128
    Image<Rgba32> normal_image = *bottom_result.value();

    // Execute operation with mismatched dimensions
    auto result = metatileizer_->metatileize(normal_image, smaller_image, normal_image);

    // Verify failure
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().details(PlainTextFormatter{}).find("mismatched dimensions") != std::string::npos);
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldFailWithInvalidDimensions)
{
    // Create images with dimensions not divisible by 16
    Image<Rgba32> invalid_image{30, 30}; // Not divisible by 16

    // Execute operation
    auto result = metatileizer_->metatileize(invalid_image, invalid_image, invalid_image);

    // Verify failure
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().details(PlainTextFormatter{}).find("failed to tileize") != std::string::npos);
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldProduceCorrectMetatileStructure)
{
    // Create a 48x48 test image (3x3 metatiles) to test row/col calculation logic
    Image<Rgba32> test_image{48, 48};

    // Create a unique color for each metatile position to verify correct positioning
    std::vector metatile_colors = {
        Rgba32{255, 0, 0},   // Metatile (0,0) - Red
        Rgba32{0, 255, 0},   // Metatile (0,1) - Green
        Rgba32{0, 0, 255},   // Metatile (0,2) - Blue
        Rgba32{255, 255, 0}, // Metatile (1,0) - Yellow
        Rgba32{255, 0, 255}, // Metatile (1,1) - Magenta
        Rgba32{0, 255, 255}, // Metatile (1,2) - Cyan
        Rgba32{128, 0, 0},   // Metatile (2,0) - Dark Red
        Rgba32{0, 128, 0},   // Metatile (2,1) - Dark Green
        Rgba32{0, 0, 128}    // Metatile (2,2) - Dark Blue
    };

    // Fill each 16x16 metatile region with its unique color
    for (std::size_t metatile_row = 0; metatile_row < 3; ++metatile_row) {
        for (std::size_t metatile_col = 0; metatile_col < 3; ++metatile_col) {
            const auto color = metatile_colors[metatile_row * 3 + metatile_col];

            // Fill the entire 16x16 region for this metatile
            for (std::size_t pixel_row = 0; pixel_row < 16; ++pixel_row) {
                for (std::size_t pixel_col = 0; pixel_col < 16; ++pixel_col) {
                    const std::size_t img_row = metatile_row * 16 + pixel_row;
                    const std::size_t img_col = metatile_col * 16 + pixel_col;
                    test_image.set(img_row, img_col, color);
                }
            }
        }
    }

    // Execute operation
    auto result = metatileizer_->metatileize(test_image, test_image, test_image);
    ASSERT_TRUE(result.has_value());

    const auto &rgba_metatiles = result.value();
    ASSERT_EQ(rgba_metatiles.size(), 9); // 3x3 = 9 metatiles

    // Verify that each metatile has the correct color based on its position
    // Metatiles are processed in row-major order: (0,0), (0,1), (0,2), (1,0), (1,1), (1,2), (2,0), (2,1), (2,2)
    for (std::size_t i = 0; i < 9; ++i) {
        const auto &metatile = rgba_metatiles[i];
        const auto expected_color = metatile_colors[i];

        // Check that all tiles in this metatile have the expected color
        // Since each metatile region was filled uniformly, every tile should have the same color
        EXPECT_EQ(metatile.bottom(0).at(0, 0), expected_color) << "Metatile " << i << " tile 0 color mismatch";
        EXPECT_EQ(metatile.bottom(1).at(0, 0), expected_color) << "Metatile " << i << " tile 1 color mismatch";
        EXPECT_EQ(metatile.bottom(2).at(0, 0), expected_color) << "Metatile " << i << " tile 2 color mismatch";
        EXPECT_EQ(metatile.bottom(3).at(0, 0), expected_color) << "Metatile " << i << " tile 3 color mismatch";
    }
}

// ====================================================================================
// Demetatileize Tests
// ====================================================================================

TEST_F(RgbaLayerImageMetatileizerTests, ShouldSuccessfullyDemetatileizeValidMetatiles)
{
    // Create a simple 32x32 test image (2x2 metatiles) with distinct colors
    Image<Rgba32> bottom_image{32, 32};
    Image<Rgba32> middle_image{32, 32};
    Image<Rgba32> top_image{32, 32};

    // Fill images with different colors for each layer
    const Rgba32 bottom_color{255, 0, 0, 255}; // Red
    const Rgba32 middle_color{0, 255, 0, 255}; // Green
    const Rgba32 top_color{0, 0, 255, 255};    // Blue

    for (std::size_t row = 0; row < 32; ++row) {
        for (std::size_t col = 0; col < 32; ++col) {
            bottom_image.set(row, col, bottom_color);
            middle_image.set(row, col, middle_color);
            top_image.set(row, col, top_color);
        }
    }

    // Metatileize the images
    auto metatileize_result = metatileizer_->metatileize(bottom_image, middle_image, top_image);
    ASSERT_TRUE(metatileize_result.has_value()) << "Failed to metatileize test images";

    const auto &metatiles = metatileize_result.value();
    EXPECT_EQ(metatiles.size(), 4); // 2x2 = 4 metatiles

    // Demetatileize back to images
    auto demetatileize_result = metatileizer_->demetatileize(metatiles, 2);
    ASSERT_TRUE(demetatileize_result.has_value())
        << "Failed to demetatileize: " << demetatileize_result.error().details(PlainTextFormatter{});

    const auto &[reconstructed_bottom, reconstructed_middle, reconstructed_top] = demetatileize_result.value();

    // Verify dimensions match
    EXPECT_EQ(reconstructed_bottom.width(), 32);
    EXPECT_EQ(reconstructed_bottom.height(), 32);
    EXPECT_EQ(reconstructed_middle.width(), 32);
    EXPECT_EQ(reconstructed_middle.height(), 32);
    EXPECT_EQ(reconstructed_top.width(), 32);
    EXPECT_EQ(reconstructed_top.height(), 32);

    // Verify pixel colors match original
    for (std::size_t row = 0; row < 32; ++row) {
        for (std::size_t col = 0; col < 32; ++col) {
            EXPECT_EQ(reconstructed_bottom.at(row, col), bottom_color)
                << "Bottom mismatch at (" << row << "," << col << ")";
            EXPECT_EQ(reconstructed_middle.at(row, col), middle_color)
                << "Middle mismatch at (" << row << "," << col << ")";
            EXPECT_EQ(reconstructed_top.at(row, col), top_color) << "Top mismatch at (" << row << "," << col << ")";
        }
    }
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldBeInverseOfMetatileize)
{
    // Load test images
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/services/bottom1.png");
    auto middle_result = loader_->load_from_file("Resources/Tests/unit/domain/services/middle1.png");
    auto top_result = loader_->load_from_file("Resources/Tests/unit/domain/services/top1.png");

    ASSERT_TRUE(bottom_result.has_value()) << "Failed to load bottom test image";
    ASSERT_TRUE(middle_result.has_value()) << "Failed to load middle test image";
    ASSERT_TRUE(top_result.has_value()) << "Failed to load top test image";

    const auto &original_bottom = *bottom_result.value();
    const auto &original_middle = *middle_result.value();
    const auto &original_top = *top_result.value();

    // Store original dimensions
    const std::size_t original_width = original_bottom.width();
    const std::size_t original_height = original_bottom.height();

    // Metatileize
    auto metatileize_result = metatileizer_->metatileize(original_bottom, original_middle, original_top);
    ASSERT_TRUE(metatileize_result.has_value()) << "Failed to metatileize original images";

    const auto &metatiles = metatileize_result.value();
    const std::size_t metatiles_per_row = original_width / metatile::side_length_pix;

    // Demetatileize
    auto demetatileize_result = metatileizer_->demetatileize(metatiles, metatiles_per_row);
    ASSERT_TRUE(demetatileize_result.has_value())
        << "Failed to demetatileize: " << demetatileize_result.error().details(PlainTextFormatter{});

    const auto &[reconstructed_bottom, reconstructed_middle, reconstructed_top] = demetatileize_result.value();

    // Verify dimensions match
    EXPECT_EQ(reconstructed_bottom.width(), original_width);
    EXPECT_EQ(reconstructed_bottom.height(), original_height);
    EXPECT_EQ(reconstructed_middle.width(), original_width);
    EXPECT_EQ(reconstructed_middle.height(), original_height);
    EXPECT_EQ(reconstructed_top.width(), original_width);
    EXPECT_EQ(reconstructed_top.height(), original_height);

    // Verify pixel-perfect reconstruction
    for (std::size_t row = 0; row < original_height; ++row) {
        for (std::size_t col = 0; col < original_width; ++col) {
            EXPECT_EQ(reconstructed_bottom.at(row, col), original_bottom.at(row, col))
                << "Bottom pixel mismatch at (" << row << "," << col << ")";
            EXPECT_EQ(reconstructed_middle.at(row, col), original_middle.at(row, col))
                << "Middle pixel mismatch at (" << row << "," << col << ")";
            EXPECT_EQ(reconstructed_top.at(row, col), original_top.at(row, col))
                << "Top pixel mismatch at (" << row << "," << col << ")";
        }
    }
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldHandleDemetatileizeWithIncompleteRows)
{
    // Create some test metatiles - 5 metatiles which will create an incomplete final row when using 2 per row
    std::vector<RgbaMetatile> metatiles(5); // 5 metatiles

    // Initialize each metatile with a test pattern so we can verify reconstruction
    const Rgba32 test_color{100, 150, 200, 255};
    for (auto &metatile : metatiles) {
        for (std::size_t tile_idx = 0; tile_idx < metatile::tiles_per_metatile; ++tile_idx) {
            // Create a test tile filled with the test color
            RgbaTile test_tile{};
            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    test_tile.set(row, col, test_color);
                }
            }
            metatile.set_bottom(tile_idx, test_tile);
            metatile.set_middle(tile_idx, test_tile);
            metatile.set_top(tile_idx, test_tile);
        }
    }

    // Try to demetatileize with 2 metatiles per row
    // This should create a 2x3 grid (3 rows) where the final row has padding
    auto result = metatileizer_->demetatileize(metatiles, 2);

    ASSERT_TRUE(result.has_value()) << "Demetatileize failed: " << result.error().details(PlainTextFormatter{});

    const auto &[bottom, middle, top] = result.value();

    // Verify dimensions: 2 metatiles per row * 16 pixels = 32 pixels wide
    // 3 rows * 16 pixels = 48 pixels tall (5 metatiles with 2 per row = ceiling(5/2) = 3 rows)
    EXPECT_EQ(bottom.width(), 32);
    EXPECT_EQ(bottom.height(), 48);

    // Verify that the first 5 metatile positions have the correct color
    // and the 6th position (bottom-right) is padded with transparent pixels
    const Rgba32 transparent{0, 0, 0, 0};

    // Check first metatile (top-left, should have test color)
    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            EXPECT_EQ(bottom.at(row, col), test_color) << "Metatile 0 color mismatch at (" << row << "," << col << ")";
        }
    }

    // Check last metatile position (bottom-right, should be transparent padding)
    for (std::size_t row = 32; row < 48; ++row) {
        for (std::size_t col = 16; col < 32; ++col) {
            EXPECT_EQ(bottom.at(row, col), transparent)
                << "Padding should be transparent at (" << row << "," << col << ")";
        }
    }
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldFailDemetatileizeWithInvalidInput)
{
    std::vector<RgbaMetatile> empty_metatiles;

    // Try with zero metatiles_per_row - this will now panic
    ASSERT_DEATH(
        std::ignore = metatileizer_->demetatileize(empty_metatiles, 0), "metatiles_per_row must be greater than zero");

    // Try with empty metatiles vector
    auto result = metatileizer_->demetatileize(empty_metatiles, 1);
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().details(PlainTextFormatter{}).find("empty") != std::string::npos);
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldHandleNonSquareImagesDemetatileize)
{
    // Create a non-square test image 32x48 (2 metatiles wide x 3 metatiles tall)
    Image<Rgba32> test_image{32, 48};

    // Create unique colors for each metatile position to verify correct ordering
    std::vector<Rgba32> metatile_colors = {
        Rgba32{255, 0, 0, 255},   // Metatile (0,0) - Red
        Rgba32{0, 255, 0, 255},   // Metatile (0,1) - Green
        Rgba32{0, 0, 255, 255},   // Metatile (1,0) - Blue
        Rgba32{255, 255, 0, 255}, // Metatile (1,1) - Yellow
        Rgba32{255, 0, 255, 255}, // Metatile (2,0) - Magenta
        Rgba32{0, 255, 255, 255}  // Metatile (2,1) - Cyan
    };

    // Fill each 16x16 metatile region with its unique color
    for (std::size_t metatile_row = 0; metatile_row < 3; ++metatile_row) {
        for (std::size_t metatile_col = 0; metatile_col < 2; ++metatile_col) {
            const auto color = metatile_colors[metatile_row * 2 + metatile_col];

            for (std::size_t pixel_row = 0; pixel_row < 16; ++pixel_row) {
                for (std::size_t pixel_col = 0; pixel_col < 16; ++pixel_col) {
                    const std::size_t img_row = metatile_row * 16 + pixel_row;
                    const std::size_t img_col = metatile_col * 16 + pixel_col;
                    test_image.set(img_row, img_col, color);
                }
            }
        }
    }

    // Metatileize
    auto metatileize_result = metatileizer_->metatileize(test_image, test_image, test_image);
    ASSERT_TRUE(metatileize_result.has_value()) << "Failed to metatileize non-square image";

    const auto &metatiles = metatileize_result.value();
    ASSERT_EQ(metatiles.size(), 6); // 2x3 = 6 metatiles

    // Verify metatiles have correct colors (tests row-major ordering)
    for (std::size_t i = 0; i < 6; ++i) {
        const auto &metatile = metatiles[i];
        const auto expected_color = metatile_colors[i];
        EXPECT_EQ(metatile.bottom(0).at(0, 0), expected_color) << "Metatile " << i << " color mismatch";
    }

    // Demetatileize
    auto demetatileize_result = metatileizer_->demetatileize(metatiles, 2);
    ASSERT_TRUE(demetatileize_result.has_value()) << "Failed to demetatileize non-square result";

    const auto &[bottom, middle, top] = demetatileize_result.value();

    // Verify dimensions
    EXPECT_EQ(bottom.width(), 32);
    EXPECT_EQ(bottom.height(), 48);

    // Verify pixel-perfect reconstruction
    for (std::size_t row = 0; row < 48; ++row) {
        for (std::size_t col = 0; col < 32; ++col) {
            EXPECT_EQ(bottom.at(row, col), test_image.at(row, col))
                << "Pixel mismatch at (" << row << "," << col << ")";
        }
    }
}

TEST_F(RgbaLayerImageMetatileizerTests, ShouldHandleSingleMetatileDemetatileize)
{
    // Create a single 16x16 test image
    Image<Rgba32> test_image{16, 16};
    const Rgba32 test_color{128, 64, 192, 255}; // Purple

    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            test_image.set(row, col, test_color);
        }
    }

    // Metatileize
    auto metatileize_result = metatileizer_->metatileize(test_image, test_image, test_image);
    ASSERT_TRUE(metatileize_result.has_value());

    const auto &metatiles = metatileize_result.value();
    EXPECT_EQ(metatiles.size(), 1); // Single metatile

    // Demetatileize
    auto demetatileize_result = metatileizer_->demetatileize(metatiles, 1);
    ASSERT_TRUE(demetatileize_result.has_value());

    const auto &[bottom, middle, top] = demetatileize_result.value();

    // Verify dimensions
    EXPECT_EQ(bottom.width(), 16);
    EXPECT_EQ(bottom.height(), 16);

    // Verify all pixels have the correct color
    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            EXPECT_EQ(bottom.at(row, col), test_color);
            EXPECT_EQ(middle.at(row, col), test_color);
            EXPECT_EQ(top.at(row, col), test_color);
        }
    }
}

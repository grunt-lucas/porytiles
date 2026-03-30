#include "gtest/gtest.h"

#include <memory>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

using namespace porytiles2;

class RgbaLayerImageMetatileizerTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        loader_ = std::make_unique<PngRgbaImageLoader>();
        metatileizer_ = std::make_unique<LayerImageMetatileizer<Rgba32>>();
    }

    std::unique_ptr<PngRgbaImageLoader> loader_;
    std::unique_ptr<LayerImageMetatileizer<Rgba32>> metatileizer_;
};

TEST_F(RgbaLayerImageMetatileizerTests, MetatileizeValidImages)
{
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/services/bottom1.png");
    auto middle_result = loader_->load_from_file("Resources/Tests/unit/domain/services/middle1.png");
    auto top_result = loader_->load_from_file("Resources/Tests/unit/domain/services/top1.png");

    ASSERT_TRUE(bottom_result.has_value()) << "Failed to load bottom test image";
    ASSERT_TRUE(middle_result.has_value()) << "Failed to load middle test image";
    ASSERT_TRUE(top_result.has_value()) << "Failed to load top test image";

    auto result = metatileizer_->metatileize(*bottom_result.value(), *middle_result.value(), *top_result.value());

    if (!result.has_value()) {
        FAIL() << "Metatileization failed: " << result.error().join(PlainTextFormatter{});
    }

    const auto &rgba_metatiles = result.value();

    EXPECT_EQ(rgba_metatiles.size(), 64);
}

TEST_F(RgbaLayerImageMetatileizerTests, MetatileizeMismatchedDimensionsFails)
{
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/services/bottom1.png");
    ASSERT_TRUE(bottom_result.has_value());

    Image<Rgba32> smaller_image{64, 64};
    Image<Rgba32> normal_image = *bottom_result.value();

    auto result = metatileizer_->metatileize(normal_image, smaller_image, normal_image);

    ASSERT_FALSE(result.has_value());
    auto details = result.error().details(PlainTextFormatter{});
    bool found = false;
    for (const auto &line : details) {
        if (line.find("mismatched dimensions") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(RgbaLayerImageMetatileizerTests, MetatileizeInvalidDimensionsFails)
{
    Image<Rgba32> invalid_image{30, 30};

    auto result = metatileizer_->metatileize(invalid_image, invalid_image, invalid_image);

    ASSERT_FALSE(result.has_value());
    auto details = result.error().details(PlainTextFormatter{});
    bool found = false;
    for (const auto &line : details) {
        if (line.find("Failed to tileize") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(RgbaLayerImageMetatileizerTests, MetatileStructure)
{
    Image<Rgba32> test_image{48, 48};

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

    auto result = metatileizer_->metatileize(test_image, test_image, test_image);
    ASSERT_TRUE(result.has_value());

    const auto &rgba_metatiles = result.value();
    ASSERT_EQ(rgba_metatiles.size(), 9);

    for (std::size_t i = 0; i < 9; ++i) {
        const auto &metatile = rgba_metatiles[i];
        const auto expected_color = metatile_colors[i];
        EXPECT_EQ(metatile.bottom(0).at(0, 0), expected_color) << "Metatile " << i << " tile 0 color mismatch";
        EXPECT_EQ(metatile.bottom(1).at(0, 0), expected_color) << "Metatile " << i << " tile 1 color mismatch";
        EXPECT_EQ(metatile.bottom(2).at(0, 0), expected_color) << "Metatile " << i << " tile 2 color mismatch";
        EXPECT_EQ(metatile.bottom(3).at(0, 0), expected_color) << "Metatile " << i << " tile 3 color mismatch";
    }
}

TEST_F(RgbaLayerImageMetatileizerTests, DemetatileizeValid)
{
    Image<Rgba32> bottom_image{32, 32};
    Image<Rgba32> middle_image{32, 32};
    Image<Rgba32> top_image{32, 32};

    const Rgba32 bottom_color{255, 0, 0, 255};
    const Rgba32 middle_color{0, 255, 0, 255};
    const Rgba32 top_color{0, 0, 255, 255};

    for (std::size_t row = 0; row < 32; ++row) {
        for (std::size_t col = 0; col < 32; ++col) {
            bottom_image.set(row, col, bottom_color);
            middle_image.set(row, col, middle_color);
            top_image.set(row, col, top_color);
        }
    }

    auto metatileize_result = metatileizer_->metatileize(bottom_image, middle_image, top_image);
    ASSERT_TRUE(metatileize_result.has_value()) << "Failed to metatileize test images";

    const auto &metatiles = metatileize_result.value();
    EXPECT_EQ(metatiles.size(), 4);

    auto demetatileize_result = metatileizer_->demetatileize(metatiles, 2);
    if (!demetatileize_result.has_value()) {
        FAIL() << "Failed to demetatileize: " << demetatileize_result.error().join(PlainTextFormatter{});
    }

    const auto &[reconstructed_bottom, reconstructed_middle, reconstructed_top] = demetatileize_result.value();

    EXPECT_EQ(reconstructed_bottom.width(), 32);
    EXPECT_EQ(reconstructed_bottom.height(), 32);
    EXPECT_EQ(reconstructed_middle.width(), 32);
    EXPECT_EQ(reconstructed_middle.height(), 32);
    EXPECT_EQ(reconstructed_top.width(), 32);
    EXPECT_EQ(reconstructed_top.height(), 32);
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

TEST_F(RgbaLayerImageMetatileizerTests, DemetatileizeInverseOfMetatileize)
{
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/services/bottom1.png");
    auto middle_result = loader_->load_from_file("Resources/Tests/unit/domain/services/middle1.png");
    auto top_result = loader_->load_from_file("Resources/Tests/unit/domain/services/top1.png");

    ASSERT_TRUE(bottom_result.has_value()) << "Failed to load bottom test image";
    ASSERT_TRUE(middle_result.has_value()) << "Failed to load middle test image";
    ASSERT_TRUE(top_result.has_value()) << "Failed to load top test image";

    const auto &original_bottom = *bottom_result.value();
    const auto &original_middle = *middle_result.value();
    const auto &original_top = *top_result.value();

    const std::size_t original_width = original_bottom.width();
    const std::size_t original_height = original_bottom.height();

    auto metatileize_result = metatileizer_->metatileize(original_bottom, original_middle, original_top);
    ASSERT_TRUE(metatileize_result.has_value()) << "Failed to metatileize original images";

    const auto &metatiles = metatileize_result.value();
    const std::size_t metatiles_per_row = original_width / metatile::side_length_pix;

    auto demetatileize_result = metatileizer_->demetatileize(metatiles, metatiles_per_row);
    if (!demetatileize_result.has_value()) {
        FAIL() << "Failed to demetatileize: " << demetatileize_result.error().join(PlainTextFormatter{});
    }

    const auto &[reconstructed_bottom, reconstructed_middle, reconstructed_top] = demetatileize_result.value();

    EXPECT_EQ(reconstructed_bottom.width(), original_width);
    EXPECT_EQ(reconstructed_bottom.height(), original_height);
    EXPECT_EQ(reconstructed_middle.width(), original_width);
    EXPECT_EQ(reconstructed_middle.height(), original_height);
    EXPECT_EQ(reconstructed_top.width(), original_width);
    EXPECT_EQ(reconstructed_top.height(), original_height);

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

TEST_F(RgbaLayerImageMetatileizerTests, DemetatileizeIncompleteRows)
{
    // Create some test metatiles - 5 metatiles which will create an incomplete final row when using 2 per row
    std::vector<Metatile<Rgba32>> metatiles(5); // 5 metatiles

    // Initialize each metatile with a test pattern so we can verify reconstruction
    const Rgba32 test_color{100, 150, 200, 255};
    for (auto &metatile : metatiles) {
        for (std::size_t tile_idx = 0; tile_idx < metatile::tiles_per_metatile_layer; ++tile_idx) {
            // Create a test tile filled with the test color
            PixelTile<Rgba32> test_tile{};
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

    if (!result.has_value()) {
        FAIL() << "Demetatileize failed: " << result.error().join(PlainTextFormatter{});
    }

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

TEST_F(RgbaLayerImageMetatileizerTests, DemetatileizeInvalidInputFails)
{
    std::vector<Metatile<Rgba32>> empty_metatiles;

    // Try with zero metatiles_per_row - this will now panic
    ASSERT_DEATH(
        std::ignore = metatileizer_->demetatileize(empty_metatiles, 0), "metatiles_per_row must be greater than zero");

    // Try with empty metatiles vector
    auto result = metatileizer_->demetatileize(empty_metatiles, 1);
    ASSERT_FALSE(result.has_value());
    auto details = result.error().details(PlainTextFormatter{});
    bool found = false;
    for (const auto &line : details) {
        if (line.find("empty") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(RgbaLayerImageMetatileizerTests, DemetatileizeNonSquare)
{
    Image<Rgba32> test_image{32, 48};

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

    auto metatileize_result = metatileizer_->metatileize(test_image, test_image, test_image);
    ASSERT_TRUE(metatileize_result.has_value()) << "Failed to metatileize non-square image";

    const auto &metatiles = metatileize_result.value();
    ASSERT_EQ(metatiles.size(), 6);

    for (std::size_t i = 0; i < 6; ++i) {
        const auto &metatile = metatiles[i];
        const auto expected_color = metatile_colors[i];
        EXPECT_EQ(metatile.bottom(0).at(0, 0), expected_color) << "Metatile " << i << " color mismatch";
    }

    auto demetatileize_result = metatileizer_->demetatileize(metatiles, 2);
    ASSERT_TRUE(demetatileize_result.has_value()) << "Failed to demetatileize non-square result";

    const auto &[bottom, middle, top] = demetatileize_result.value();

    EXPECT_EQ(bottom.width(), 32);
    EXPECT_EQ(bottom.height(), 48);
    for (std::size_t row = 0; row < 48; ++row) {
        for (std::size_t col = 0; col < 32; ++col) {
            EXPECT_EQ(bottom.at(row, col), test_image.at(row, col))
                << "Pixel mismatch at (" << row << "," << col << ")";
        }
    }
}

TEST_F(RgbaLayerImageMetatileizerTests, DemetatileizeSingleMetatile)
{
    Image<Rgba32> test_image{16, 16};
    const Rgba32 test_color{128, 64, 192, 255};

    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            test_image.set(row, col, test_color);
        }
    }

    auto metatileize_result = metatileizer_->metatileize(test_image, test_image, test_image);
    ASSERT_TRUE(metatileize_result.has_value());

    const auto &metatiles = metatileize_result.value();
    EXPECT_EQ(metatiles.size(), 1);

    auto demetatileize_result = metatileizer_->demetatileize(metatiles, 1);
    ASSERT_TRUE(demetatileize_result.has_value());

    const auto &[bottom, middle, top] = demetatileize_result.value();

    EXPECT_EQ(bottom.width(), 16);
    EXPECT_EQ(bottom.height(), 16);
    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            EXPECT_EQ(bottom.at(row, col), test_color);
            EXPECT_EQ(middle.at(row, col), test_color);
            EXPECT_EQ(top.at(row, col), test_color);
        }
    }
}

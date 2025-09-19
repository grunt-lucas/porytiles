#include "gtest/gtest.h"

#include <memory>

#include "porytiles2/domain/compile_tileset/operations/construct_rgba_metatiles_op.hpp"
#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_metatile.hpp"
#include "porytiles2/domain/orchestration/operand_bundle.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"

using namespace porytiles2;

class ConstructRgbaMetatilesOpTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        loader_ = std::make_unique<PngRgbaImageLoader>();
        op_ = std::make_unique<ConstructRgbaMetatilesOp>();
    }

    std::unique_ptr<PngRgbaImageLoader> loader_;
    std::unique_ptr<ConstructRgbaMetatilesOp> op_;
};

TEST_F(ConstructRgbaMetatilesOpTests, ShouldSuccessfullyConstructMetatilesFromValidImages)
{
    // Load test images
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/compile_tileset/operations/bottom1.png");
    auto middle_result = loader_->load_from_file("Resources/Tests/unit/domain/compile_tileset/operations/middle1.png");
    auto top_result = loader_->load_from_file("Resources/Tests/unit/domain/compile_tileset/operations/top1.png");

    ASSERT_TRUE(bottom_result.has_value()) << "Failed to load bottom test image";
    ASSERT_TRUE(middle_result.has_value()) << "Failed to load middle test image";
    ASSERT_TRUE(top_result.has_value()) << "Failed to load top test image";

    // Create input bundle
    OperandBundle inputs;
    inputs.put("bottom.png", *bottom_result.value());
    inputs.put("middle.png", *middle_result.value());
    inputs.put("top.png", *top_result.value());

    // Execute operation
    auto result = op_->apply(inputs);

    // Verify success
    ASSERT_TRUE(result.has_value()) << "Operation failed: " << result.error();

    // Get the result
    auto rgba_metatiles_opt = result.value().get_unwrapped<std::vector<RgbaMetatile>>("rgba_metatiles");
    ASSERT_TRUE(rgba_metatiles_opt.has_value()) << "Failed to get rgba_metatiles from result";

    const auto &rgba_metatiles = rgba_metatiles_opt.value();

    // Verify expected number of metatiles (128x128 image = 8x8 metatiles = 64 total)
    EXPECT_EQ(rgba_metatiles.size(), 64);
}

TEST_F(ConstructRgbaMetatilesOpTests, ShouldFailWithMismatchedImageDimensions)
{
    // Load test images
    auto bottom_result = loader_->load_from_file("Resources/Tests/unit/domain/compile_tileset/operations/bottom1.png");
    ASSERT_TRUE(bottom_result.has_value());

    // Create images with different dimensions
    Image<Rgba32> smaller_image{64, 64}; // Different from 128x128
    Image<Rgba32> normal_image = *bottom_result.value();

    // Create input bundle with mismatched dimensions
    OperandBundle inputs;
    inputs.put("bottom.png", normal_image);
    inputs.put("middle.png", smaller_image);
    inputs.put("top.png", normal_image);

    // Execute operation
    auto result = op_->apply(inputs);

    // Verify failure
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("mismatched dimensions") != std::string::npos);
}

TEST_F(ConstructRgbaMetatilesOpTests, ShouldFailWithInvalidDimensions)
{
    // Create images with dimensions not divisible by 16
    Image<Rgba32> invalid_image{30, 30}; // Not divisible by 16

    // Create input bundle
    OperandBundle inputs;
    inputs.put("bottom.png", invalid_image);
    inputs.put("middle.png", invalid_image);
    inputs.put("top.png", invalid_image);

    // Execute operation
    auto result = op_->apply(inputs);

    // Verify failure
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("must be multiples of") != std::string::npos);
}

TEST_F(ConstructRgbaMetatilesOpTests, ShouldProduceCorrectMetatileStructure)
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

    // Create input bundle
    OperandBundle inputs;
    inputs.put("bottom.png", test_image);
    inputs.put("middle.png", test_image);
    inputs.put("top.png", test_image);

    // Execute operation
    auto result = op_->apply(inputs);
    ASSERT_TRUE(result.has_value());

    auto rgba_metatiles_opt = result.value().get_unwrapped<std::vector<RgbaMetatile>>("rgba_metatiles");
    ASSERT_TRUE(rgba_metatiles_opt.has_value());

    const auto &rgba_metatiles = rgba_metatiles_opt.value();
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

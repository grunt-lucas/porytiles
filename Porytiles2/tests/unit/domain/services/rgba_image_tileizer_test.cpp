#include "gtest/gtest.h"

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/services/rgba_image_tileizer.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

using namespace porytiles2;

class RgbaImageTileizerTests : public ::testing::Test {
  protected:
    RgbaImageTileizer tileizer_;
};

TEST_F(RgbaImageTileizerTests, TileizeValidSingleTileImage)
{
    // Create an 8x8 image (single tile)
    Image<Rgba32> img{8, 8};

    // Fill with test data - red in top-left corner, blue elsewhere
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            if (row == 0 && col == 0) {
                img.set(row, col, Rgba32{255, 0, 0, 255}); // Red
            }
            else {
                img.set(row, col, Rgba32{0, 0, 255, 255}); // Blue
            }
        }
    }

    const auto result = tileizer_.tileize(img);
    ASSERT_TRUE(result.has_value());
    const auto tiles = result.value();

    ASSERT_EQ(tiles.size(), 1);

    // Check the first pixel is red
    const auto red_pixel = tiles[0].at(0, 0);
    EXPECT_EQ(red_pixel.red(), 255);
    EXPECT_EQ(red_pixel.green(), 0);
    EXPECT_EQ(red_pixel.blue(), 0);
    EXPECT_EQ(red_pixel.alpha(), 255);

    // Check a non-corner pixel is blue
    const auto blue_pixel = tiles[0].at(1, 1);
    EXPECT_EQ(blue_pixel.red(), 0);
    EXPECT_EQ(blue_pixel.green(), 0);
    EXPECT_EQ(blue_pixel.blue(), 255);
    EXPECT_EQ(blue_pixel.alpha(), 255);
}

TEST_F(RgbaImageTileizerTests, TileizeValidMultiTileImage)
{
    // Create a 16x8 image (2 horizontal tiles)
    Image<Rgba32> img{16, 8};

    // Fill first tile with red, second tile with green
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            if (col < 8) {
                img.set(row, col, Rgba32{255, 0, 0, 255}); // Red for first tile
            }
            else {
                img.set(row, col, Rgba32{0, 255, 0, 255}); // Green for second tile
            }
        }
    }

    const auto result = tileizer_.tileize(img);
    ASSERT_TRUE(result.has_value());
    const auto tiles = result.value();

    ASSERT_EQ(tiles.size(), 2);

    // Check first tile is red
    const auto red_pixel = tiles[0].at(4, 4);
    EXPECT_EQ(red_pixel.red(), 255);
    EXPECT_EQ(red_pixel.green(), 0);
    EXPECT_EQ(red_pixel.blue(), 0);

    // Check second tile is green
    const auto green_pixel = tiles[1].at(4, 4);
    EXPECT_EQ(green_pixel.red(), 0);
    EXPECT_EQ(green_pixel.green(), 255);
    EXPECT_EQ(green_pixel.blue(), 0);
}

TEST_F(RgbaImageTileizerTests, TileizeValidQuadTileImage)
{
    // Create a 16x16 image (4 tiles in 2x2 arrangement)
    Image<Rgba32> img{16, 16};

    // Fill quadrants with different colors
    for (std::size_t row = 0; row < 16; ++row) {
        for (std::size_t col = 0; col < 16; ++col) {
            if (row < 8 && col < 8) {
                img.set(row, col, Rgba32{255, 0, 0, 255}); // Red (top-left)
            }
            else if (row < 8 && col >= 8) {
                img.set(row, col, Rgba32{0, 255, 0, 255}); // Green (top-right)
            }
            else if (row >= 8 && col < 8) {
                img.set(row, col, Rgba32{0, 0, 255, 255}); // Blue (bottom-left)
            }
            else {
                img.set(row, col, Rgba32{255, 255, 0, 255}); // Yellow (bottom-right)
            }
        }
    }

    const auto result = tileizer_.tileize(img);
    ASSERT_TRUE(result.has_value());
    const auto tiles = result.value();

    ASSERT_EQ(tiles.size(), 4);

    // Check tile ordering: top-left, top-right, bottom-left, bottom-right
    const auto red_pixel = tiles[0].at(4, 4);
    EXPECT_EQ(red_pixel.red(), 255);
    EXPECT_EQ(red_pixel.green(), 0);
    EXPECT_EQ(red_pixel.blue(), 0);

    const auto green_pixel = tiles[1].at(4, 4);
    EXPECT_EQ(green_pixel.red(), 0);
    EXPECT_EQ(green_pixel.green(), 255);
    EXPECT_EQ(green_pixel.blue(), 0);

    const auto blue_pixel = tiles[2].at(4, 4);
    EXPECT_EQ(blue_pixel.red(), 0);
    EXPECT_EQ(blue_pixel.green(), 0);
    EXPECT_EQ(blue_pixel.blue(), 255);

    const auto yellow_pixel = tiles[3].at(4, 4);
    EXPECT_EQ(yellow_pixel.red(), 255);
    EXPECT_EQ(yellow_pixel.green(), 255);
    EXPECT_EQ(yellow_pixel.blue(), 0);
}

TEST_F(RgbaImageTileizerTests, TileizeInvalidWidthReturnsError)
{
    // Create an image with width not divisible by 8
    Image<Rgba32> img{9, 8};

    const auto result = tileizer_.tileize(img);
    EXPECT_FALSE(result.has_value());
}

TEST_F(RgbaImageTileizerTests, TileizeInvalidHeightReturnsError)
{
    // Create an image with height not divisible by 8
    Image<Rgba32> img{8, 9};

    const auto result = tileizer_.tileize(img);
    EXPECT_FALSE(result.has_value());
}

TEST_F(RgbaImageTileizerTests, TileizeInvalidBothDimensionsReturnsError)
{
    // Create an image with neither dimension divisible by 8
    Image<Rgba32> img{7, 7};

    const auto result = tileizer_.tileize(img);
    EXPECT_FALSE(result.has_value());
}

TEST_F(RgbaImageTileizerTests, TileizeEmptyImageReturnsEmptyVector)
{
    // Create a 0x0 image
    Image<Rgba32> img{0, 0};

    const auto result = tileizer_.tileize(img);
    ASSERT_TRUE(result.has_value());
    const auto tiles = result.value();

    EXPECT_TRUE(tiles.empty());
}

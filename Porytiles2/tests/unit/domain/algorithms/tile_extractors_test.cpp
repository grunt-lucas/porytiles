#include <gtest/gtest.h>

#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

// ==================================================
// extract_tiles_from_image (full image) tests
// ==================================================

TEST(TileExtractorsTests, ExtractTilesFromImage_SingleTile)
{
    // Arrange: Create an 8x8 image (one tile)
    Image<Rgba32> img{8, 8};
    img.set(0, 0, rgba_red);
    img.set(0, 7, rgba_green);
    img.set(7, 0, rgba_blue);
    img.set(7, 7, rgba_yellow);

    // Act
    auto tiles = extract_tiles_from_image(img);

    // Assert
    ASSERT_EQ(tiles.size(), 1);
    EXPECT_EQ(tiles[0].at(0, 0), rgba_red);
    EXPECT_EQ(tiles[0].at(0, 7), rgba_green);
    EXPECT_EQ(tiles[0].at(7, 0), rgba_blue);
    EXPECT_EQ(tiles[0].at(7, 7), rgba_yellow);
}

TEST(TileExtractorsTests, ExtractTilesFromImage_TwoByTwoGrid)
{
    // Arrange: Create a 16x16 image (2x2 tiles)
    Image<Rgba32> img{16, 16};

    // Mark corners of each tile with distinct colors
    // Tile 0 (top-left)
    img.set(0, 0, rgba_red);
    // Tile 1 (top-right)
    img.set(0, 8, rgba_green);
    // Tile 2 (bottom-left)
    img.set(8, 0, rgba_blue);
    // Tile 3 (bottom-right)
    img.set(8, 8, rgba_yellow);

    // Act
    auto tiles = extract_tiles_from_image(img);

    // Assert: Should have 4 tiles in row-major order
    ASSERT_EQ(tiles.size(), 4);

    // Verify each tile has the correct corner pixel
    EXPECT_EQ(tiles[0].at(0, 0), rgba_red);    // Tile 0
    EXPECT_EQ(tiles[1].at(0, 0), rgba_green);  // Tile 1
    EXPECT_EQ(tiles[2].at(0, 0), rgba_blue);   // Tile 2
    EXPECT_EQ(tiles[3].at(0, 0), rgba_yellow); // Tile 3
}

TEST(TileExtractorsTests, ExtractTilesFromImage_RowMajorOrder)
{
    // Arrange: Create a 24x8 image (3 tiles in a row)
    Image<IndexPixel> img{24, 8};

    // Mark first pixel of each tile with its index
    img.set(0, 0, IndexPixel{1});  // Tile 0
    img.set(0, 8, IndexPixel{2});  // Tile 1
    img.set(0, 16, IndexPixel{3}); // Tile 2

    // Act
    auto tiles = extract_tiles_from_image(img);

    // Assert
    ASSERT_EQ(tiles.size(), 3);
    EXPECT_EQ(tiles[0].at(0, 0).index(), 1);
    EXPECT_EQ(tiles[1].at(0, 0).index(), 2);
    EXPECT_EQ(tiles[2].at(0, 0).index(), 3);
}

TEST(TileExtractorsTests, ExtractTilesFromImage_PreservesAllPixels)
{
    // Arrange: Create an 8x8 image with a unique value in each pixel
    Image<IndexPixel> img{8, 8};
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            img.set(row, col, IndexPixel{row * 8 + col});
        }
    }

    // Act
    auto tiles = extract_tiles_from_image(img);

    // Assert: Single tile should preserve all pixel values
    ASSERT_EQ(tiles.size(), 1);
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            EXPECT_EQ(tiles[0].at(row, col).index(), row * 8 + col);
        }
    }
}

TEST(TileExtractorsTests, ExtractTilesFromImage_EmptyImage)
{
    // Arrange: Create an empty image (0x0)
    Image<Rgba32> img{0, 0};

    // Act
    auto tiles = extract_tiles_from_image(img);

    // Assert
    EXPECT_TRUE(tiles.empty());
}

TEST(TileExtractorsTests, ExtractTilesFromImage_NonMultipleOf8Width_Panics)
{
    // Arrange: Create an image with invalid width
    Image<Rgba32> img{10, 8};

    // Act & Assert
    EXPECT_DEATH({ std::ignore = extract_tiles_from_image(img); }, "image dimensions must be multiples of 8");
}

TEST(TileExtractorsTests, ExtractTilesFromImage_NonMultipleOf8Height_Panics)
{
    // Arrange: Create an image with invalid height
    Image<Rgba32> img{8, 12};

    // Act & Assert
    EXPECT_DEATH({ std::ignore = extract_tiles_from_image(img); }, "image dimensions must be multiples of 8");
}

TEST(TileExtractorsTests, ExtractTilesFromImage_LargeImage)
{
    // Arrange: Create a 128x64 image (16x8 = 128 tiles)
    Image<IndexPixel> img{128, 64};

    // Mark first pixel of each tile
    for (std::size_t tile_row = 0; tile_row < 8; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < 16; ++tile_col) {
            std::size_t tile_idx = tile_row * 16 + tile_col;
            img.set(tile_row * 8, tile_col * 8, IndexPixel{tile_idx % 256});
        }
    }

    // Act
    auto tiles = extract_tiles_from_image(img);

    // Assert
    ASSERT_EQ(tiles.size(), 128);

    // Verify a few tiles
    EXPECT_EQ(tiles[0].at(0, 0).index(), 0);
    EXPECT_EQ(tiles[15].at(0, 0).index(), 15);
    EXPECT_EQ(tiles[16].at(0, 0).index(), 16);
    EXPECT_EQ(tiles[127].at(0, 0).index(), 127);
}

// ==================================================
// extract_tiles_from_image (offset/count overload) tests
// ==================================================

TEST(TileExtractorsTests, ExtractTilesWithOffset_SingleTile)
{
    // Arrange: Create a 128x8 image (16 tiles in a row, like tiles.png)
    Image<IndexPixel> img{128, 8};

    // Mark each tile with its index
    for (std::size_t i = 0; i < 16; ++i) {
        img.set(0, i * 8, IndexPixel{i});
    }

    // Act: Extract tile at offset 5
    auto tiles = extract_tiles_from_image(img, 5, 1, 16);

    // Assert
    ASSERT_EQ(tiles.size(), 1);
    EXPECT_EQ(tiles[0].at(0, 0).index(), 5);
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_MultipleTiles)
{
    // Arrange: Create a 128x8 image (16 tiles)
    Image<IndexPixel> img{128, 8};

    for (std::size_t i = 0; i < 16; ++i) {
        img.set(0, i * 8, IndexPixel{i});
    }

    // Act: Extract 3 tiles starting at offset 2
    auto tiles = extract_tiles_from_image(img, 2, 3, 16);

    // Assert
    ASSERT_EQ(tiles.size(), 3);
    EXPECT_EQ(tiles[0].at(0, 0).index(), 2);
    EXPECT_EQ(tiles[1].at(0, 0).index(), 3);
    EXPECT_EQ(tiles[2].at(0, 0).index(), 4);
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_CrossingRows)
{
    // Arrange: Create a 128x16 image (16 tiles per row, 2 rows = 32 tiles total)
    Image<IndexPixel> img{128, 16};

    // Mark each tile with its linear index
    for (std::size_t tile_row = 0; tile_row < 2; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < 16; ++tile_col) {
            std::size_t idx = tile_row * 16 + tile_col;
            img.set(tile_row * 8, tile_col * 8, IndexPixel{idx});
        }
    }

    // Act: Extract 4 tiles starting at offset 14 (should cross row boundary)
    auto tiles = extract_tiles_from_image(img, 14, 4, 16);

    // Assert
    ASSERT_EQ(tiles.size(), 4);
    EXPECT_EQ(tiles[0].at(0, 0).index(), 14); // Last two tiles of first row
    EXPECT_EQ(tiles[1].at(0, 0).index(), 15);
    EXPECT_EQ(tiles[2].at(0, 0).index(), 16); // First two tiles of second row
    EXPECT_EQ(tiles[3].at(0, 0).index(), 17);
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_ZeroTiles)
{
    // Arrange
    Image<Rgba32> img{128, 8};

    // Act: Extract 0 tiles
    auto tiles = extract_tiles_from_image(img, 5, 0, 16);

    // Assert
    EXPECT_TRUE(tiles.empty());
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_StartAtZero)
{
    // Arrange: Create a 128x8 image
    Image<IndexPixel> img{128, 8};

    for (std::size_t i = 0; i < 16; ++i) {
        img.set(0, i * 8, IndexPixel{i});
    }

    // Act: Extract from offset 0
    auto tiles = extract_tiles_from_image(img, 0, 3, 16);

    // Assert
    ASSERT_EQ(tiles.size(), 3);
    EXPECT_EQ(tiles[0].at(0, 0).index(), 0);
    EXPECT_EQ(tiles[1].at(0, 0).index(), 1);
    EXPECT_EQ(tiles[2].at(0, 0).index(), 2);
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_CustomTilesPerRow)
{
    // Arrange: Create a 64x16 image (8 tiles per row, 2 rows)
    Image<IndexPixel> img{64, 16};

    // Mark each tile
    for (std::size_t tile_row = 0; tile_row < 2; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < 8; ++tile_col) {
            std::size_t idx = tile_row * 8 + tile_col;
            img.set(tile_row * 8, tile_col * 8, IndexPixel{idx});
        }
    }

    // Act: Extract with tiles_per_row=8
    auto tiles = extract_tiles_from_image(img, 6, 4, 8);

    // Assert: Should get tiles 6, 7, 8, 9
    ASSERT_EQ(tiles.size(), 4);
    EXPECT_EQ(tiles[0].at(0, 0).index(), 6); // Last two of first row
    EXPECT_EQ(tiles[1].at(0, 0).index(), 7);
    EXPECT_EQ(tiles[2].at(0, 0).index(), 8); // First two of second row
    EXPECT_EQ(tiles[3].at(0, 0).index(), 9);
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_PreservesAllPixels)
{
    // Arrange: Create a 128x8 image with a pattern
    Image<IndexPixel> img{128, 8};

    // Fill tile 5 with a pattern
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            img.set(row, 5 * 8 + col, IndexPixel{row * 8 + col});
        }
    }

    // Act: Extract tile 5
    auto tiles = extract_tiles_from_image(img, 5, 1, 16);

    // Assert: All 64 pixels should match
    ASSERT_EQ(tiles.size(), 1);
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            EXPECT_EQ(tiles[0].at(row, col).index(), row * 8 + col);
        }
    }
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_Rgba32Type)
{
    // Arrange: Create a 128x8 Rgba32 image
    Image<Rgba32> img{128, 8};

    img.set(0, 0, rgba_red);    // Tile 0
    img.set(0, 24, rgba_green); // Tile 3
    img.set(0, 40, rgba_blue);  // Tile 5

    // Act: Extract tiles 3-5
    auto tiles = extract_tiles_from_image(img, 3, 3, 16);

    // Assert
    ASSERT_EQ(tiles.size(), 3);
    EXPECT_EQ(tiles[0].at(0, 0), rgba_green); // Tile 3
    EXPECT_EQ(tiles[2].at(0, 0), rgba_blue);  // Tile 5
}

TEST(TileExtractorsTests, ExtractTilesWithOffset_NonMultipleOf8_Panics)
{
    // Arrange: Invalid image dimensions
    Image<Rgba32> img{130, 8};

    // Act & Assert
    EXPECT_DEATH({ std::ignore = extract_tiles_from_image(img, 0, 1, 16); }, "image dimensions must be multiples of 8");
}

// ==================================================
// Comparison with full extraction
// ==================================================

TEST(TileExtractorsTests, OffsetExtraction_MatchesFullExtraction)
{
    // Arrange: Create a 128x16 image
    Image<IndexPixel> img{128, 16};

    // Fill with pattern
    for (std::size_t tile_row = 0; tile_row < 2; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < 16; ++tile_col) {
            std::size_t tile_idx = tile_row * 16 + tile_col;
            for (std::size_t row = 0; row < 8; ++row) {
                for (std::size_t col = 0; col < 8; ++col) {
                    img.set(tile_row * 8 + row, tile_col * 8 + col, IndexPixel{(tile_idx + row + col) % 256});
                }
            }
        }
    }

    // Act: Extract all tiles both ways
    auto all_tiles = extract_tiles_from_image(img);
    auto offset_tiles = extract_tiles_from_image(img, 0, 32, 16);

    // Assert: Should be identical
    ASSERT_EQ(all_tiles.size(), offset_tiles.size());
    for (std::size_t i = 0; i < all_tiles.size(); ++i) {
        EXPECT_EQ(all_tiles[i], offset_tiles[i]) << "Mismatch at tile " << i;
    }
}

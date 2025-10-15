#include "gtest/gtest.h"

#include "porytiles2/domain/models/tiles_png_workspace.hpp"

using namespace porytiles2;

// ========================================
// First Constructor Tests
// ========================================

TEST(TilesPngWorkspaceTests, FirstConstructorShouldInitializeWithCorrectCapacity)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    EXPECT_EQ(workspace.capacity(), capacity);
}

TEST(TilesPngWorkspaceTests, FirstConstructorShouldInitializeAllTilesAsTransparent)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    // All tiles should be transparent
    for (std::size_t i = 0; i < capacity; ++i) {
        EXPECT_TRUE(workspace.tile_at(i).is_transparent());
    }
}

TEST(TilesPngWorkspaceTests, FirstConstructorShouldNotBeAtCapacityInitially)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    EXPECT_FALSE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, FirstConstructorWithCapacityOne)
{
    constexpr std::size_t capacity = 1;
    TilesPngWorkspace workspace{capacity};

    EXPECT_EQ(workspace.capacity(), capacity);
    EXPECT_TRUE(workspace.tile_at(0).is_transparent());
    EXPECT_TRUE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, FirstConstructorWithLargeCapacity)
{
    constexpr std::size_t capacity = 1024;
    TilesPngWorkspace workspace{capacity};

    EXPECT_EQ(workspace.capacity(), capacity);
    EXPECT_FALSE(workspace.at_capacity());
}

// ========================================
// Second Constructor Tests
// ========================================

TEST(TilesPngWorkspaceTests, SecondConstructorShouldLoadValidImage)
{
    // Create a simple 8x8 image (1 tile)
    Image<IndexPixel> img{8, 8};

    // Set some non-transparent pixels
    img.set(0, 0, IndexPixel{1});
    img.set(1, 1, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    EXPECT_EQ(workspace.capacity(), 10);
    EXPECT_FALSE(workspace.at_capacity());

    // First tile should not be transparent
    EXPECT_FALSE(workspace.tile_at(0).is_transparent());

    // Remaining tiles should be transparent
    for (std::size_t i = 1; i < 10; ++i) {
        EXPECT_TRUE(workspace.tile_at(i).is_transparent());
    }
}

TEST(TilesPngWorkspaceTests, SecondConstructorShouldExtractMultipleTiles)
{
    // Create a 16x8 image (2 tiles)
    Image<IndexPixel> img{16, 8};

    // First tile (0-7, 0-7)
    img.set(0, 0, IndexPixel{1});

    // Second tile (0-7, 8-15)
    img.set(0, 8, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    EXPECT_EQ(workspace.capacity(), 10);

    // Both tiles should be non-transparent
    EXPECT_FALSE(workspace.tile_at(0).is_transparent());
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());
}

TEST(TilesPngWorkspaceTests, SecondConstructorShouldHandleAllTransparentImage)
{
    // Create a 16x16 image (4 tiles) with all transparent pixels
    Image<IndexPixel> img{16, 16};

    TilesPngWorkspace workspace{img, 10};

    EXPECT_EQ(workspace.capacity(), 10);

    // All tiles should be transparent
    for (std::size_t i = 0; i < 10; ++i) {
        EXPECT_TRUE(workspace.tile_at(i).is_transparent());
    }

    // Cursor should be at position 1 (first available after tile 0)
    EXPECT_FALSE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, SecondConstructorShouldPanicOnInvalidWidthDimension)
{
    // Create an image with width not a multiple of 8
    Image<IndexPixel> img{7, 8};

    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 10), "Image dimensions must be a multiple of 8");
}

TEST(TilesPngWorkspaceTests, SecondConstructorShouldPanicOnInvalidHeightDimension)
{
    // Create an image with height not a multiple of 8
    Image<IndexPixel> img{8, 7};

    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 10), "Image dimensions must be a multiple of 8");
}

TEST(TilesPngWorkspaceTests, SecondConstructorShouldPanicOnInvalidBothDimensions)
{
    // Create an image with both dimensions not multiples of 8
    Image<IndexPixel> img{15, 15};

    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 10), "Image dimensions must be a multiple of 8");
}

TEST(TilesPngWorkspaceTests, SecondConstructorShouldPanicIfImageExceedsCapacity)
{
    // Create a 16x16 image (4 tiles)
    Image<IndexPixel> img{16, 16};

    // Try to create workspace with capacity less than needed
    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 3), "Image contains 4 tiles but capacity is only 3");
}

TEST(TilesPngWorkspaceTests, SecondConstructorShouldHandleExactCapacityMatch)
{
    // Create a 16x16 image (4 tiles)
    Image<IndexPixel> img{16, 16};
    img.set(0, 0, IndexPixel{1});

    // Create workspace with exact capacity
    TilesPngWorkspace workspace{img, 4};

    EXPECT_EQ(workspace.capacity(), 4);
}

// ========================================
// insert_tile Tests
// ========================================

TEST(TilesPngWorkspaceTests, InsertTileShouldInsertNonTransparentTile)
{
    TilesPngWorkspace workspace{10};

    // Create a non-transparent tile
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    bool result = workspace.insert_tile(tile);

    EXPECT_TRUE(result);
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());
}

TEST(TilesPngWorkspaceTests, InsertTileShouldReturnFalseForTransparentTile)
{
    TilesPngWorkspace workspace{10};

    // Create a transparent tile
    PixelTile<IndexPixel> pixel_tile;
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    bool result = workspace.insert_tile(tile);

    EXPECT_FALSE(result);
}

TEST(TilesPngWorkspaceTests, InsertTileShouldReturnFalseWhenAtCapacity)
{
    TilesPngWorkspace workspace{1};

    // Create a non-transparent tile
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    // Workspace with capacity 1 should be at capacity initially (tile 0 is reserved)
    EXPECT_TRUE(workspace.at_capacity());

    bool result = workspace.insert_tile(tile);

    EXPECT_FALSE(result);
}

TEST(TilesPngWorkspaceTests, InsertTileShouldFillCapacityProperly)
{
    constexpr std::size_t capacity = 5;
    TilesPngWorkspace workspace{capacity};

    // Insert tiles until at capacity
    for (std::size_t i = 1; i < capacity; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{static_cast<unsigned int>(i)});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};

        bool result = workspace.insert_tile(tile);
        EXPECT_TRUE(result);
    }

    // Now should be at capacity
    EXPECT_TRUE(workspace.at_capacity());

    // Try to insert one more
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{99});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    bool result = workspace.insert_tile(tile);
    EXPECT_FALSE(result);
}

TEST(TilesPngWorkspaceTests, InsertTileShouldFastForwardCursorCorrectly)
{
    TilesPngWorkspace workspace{10};

    // Insert a few non-transparent tiles
    for (std::size_t i = 0; i < 3; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{static_cast<unsigned int>(i + 1)});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};

        std::ignore = workspace.insert_tile(tile);
    }

    EXPECT_FALSE(workspace.at_capacity());

    // Tiles 1, 2, 3 should be non-transparent
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());
    EXPECT_FALSE(workspace.tile_at(2).is_transparent());
    EXPECT_FALSE(workspace.tile_at(3).is_transparent());

    // Tiles 4+ should still be transparent
    EXPECT_TRUE(workspace.tile_at(4).is_transparent());
}

// ========================================
// first_occurrence_of Tests
// ========================================

TEST(TilesPngWorkspaceTests, FirstOccurrenceOfShouldFindInsertedTile)
{
    TilesPngWorkspace workspace{10};

    // Create and insert a tile
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{5});
    pixel_tile.set(1, 1, IndexPixel{7});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::ignore = workspace.insert_tile(tile);

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 1);
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceOfShouldReturnNulloptForNonExistentTile)
{
    TilesPngWorkspace workspace{10};

    // Create a tile that was never inserted
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{99});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_FALSE(occurrence.has_value());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceOfShouldReturnNulloptForTransparentTile)
{
    TilesPngWorkspace workspace{10};

    // Create a transparent tile
    PixelTile<IndexPixel> pixel_tile;
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_FALSE(occurrence.has_value());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceOfShouldFindTileFromImage)
{
    // Create a 8x8 image with a specific pattern
    Image<IndexPixel> img{8, 8};
    img.set(0, 0, IndexPixel{1});
    img.set(1, 1, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    // Create the same tile pattern
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    pixel_tile.set(1, 1, IndexPixel{2});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 0);
}

// ========================================
// tile_at Tests
// ========================================

TEST(TilesPngWorkspaceTests, TileAtShouldReturnCorrectTile)
{
    TilesPngWorkspace workspace{10};

    // Insert a specific tile
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    pixel_tile.set(1, 1, IndexPixel{2});
    pixel_tile.set(2, 2, IndexPixel{3});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::ignore = workspace.insert_tile(tile);

    auto retrieved_tile = workspace.tile_at(1);
    // Verify the tile is not transparent and matches the inserted tile
    EXPECT_FALSE(retrieved_tile.is_transparent());
    EXPECT_EQ(retrieved_tile, tile);
}

TEST(TilesPngWorkspaceTests, TileAtShouldPanicOnOutOfBounds)
{
    TilesPngWorkspace workspace{10};

    EXPECT_DEATH(std::ignore = workspace.tile_at(10), "index 10 >= size 10");
}

TEST(TilesPngWorkspaceTests, TileAtShouldWorkForAllValidIndices)
{
    constexpr std::size_t capacity = 5;
    TilesPngWorkspace workspace{capacity};

    // Should not panic for any valid index
    for (std::size_t i = 0; i < capacity; ++i) {
        auto tile = workspace.tile_at(i);
        EXPECT_TRUE(tile.is_transparent());
    }
}

// ========================================
// at_capacity Tests
// ========================================

TEST(TilesPngWorkspaceTests, AtCapacityShouldReturnFalseInitially)
{
    TilesPngWorkspace workspace{10};
    EXPECT_FALSE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, AtCapacityShouldReturnTrueWhenFull)
{
    constexpr std::size_t capacity = 3;
    TilesPngWorkspace workspace{capacity};

    // Fill workspace
    for (std::size_t i = 1; i < capacity; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{static_cast<unsigned int>(i)});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};
        std::ignore = workspace.insert_tile(tile);
    }

    EXPECT_TRUE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, AtCapacityShouldReturnTrueForCapacityOne)
{
    TilesPngWorkspace workspace{1};
    EXPECT_TRUE(workspace.at_capacity());
}

// ========================================
// Integration Tests
// ========================================

TEST(TilesPngWorkspaceTests, ShouldHandleComplexWorkflow)
{
    // Create an image with 2 tiles
    Image<IndexPixel> img{16, 8};
    img.set(0, 0, IndexPixel{1});
    img.set(0, 8, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    // Verify loaded tiles
    EXPECT_FALSE(workspace.tile_at(0).is_transparent());
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());

    // Insert additional tiles
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{3});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    bool result = workspace.insert_tile(tile);
    EXPECT_TRUE(result);

    // Verify the inserted tile
    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 2);

    EXPECT_FALSE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, ShouldDeduplicateTilesFromImage)
{
    // Create an image with identical tiles
    Image<IndexPixel> img{16, 8};

    // Set both tiles to have the same pattern
    img.set(0, 0, IndexPixel{5});
    img.set(0, 8, IndexPixel{5});

    TilesPngWorkspace workspace{img, 10};

    // Create the same tile pattern
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{5});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    // Should find the first occurrence
    EXPECT_EQ(occurrence.value(), 0);
}

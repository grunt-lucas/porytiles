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

// ========================================
// Export Tests
// ========================================

TEST(TilesPngWorkspaceTests, ExportImageShouldHaveCorrectDimensions)
{
    TilesPngWorkspace workspace{256};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    // Standard tiles.png format: 128 pixels wide (16 tiles)
    EXPECT_EQ(img.width(), 128);
    // 256 tiles / 16 tiles per row = 16 rows * 8 pixels per tile = 128 pixels
    EXPECT_EQ(img.height(), 128);
}

TEST(TilesPngWorkspaceTests, ExportImageShouldCalculateHeightForNonSquareCapacity)
{
    TilesPngWorkspace workspace{32};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    // Standard tiles.png format: 128 pixels wide (16 tiles)
    EXPECT_EQ(img.width(), 128);
    // 32 tiles / 16 tiles per row = 2 rows * 8 pixels per tile = 16 pixels
    EXPECT_EQ(img.height(), 16);
}

TEST(TilesPngWorkspaceTests, ExportImageShouldHandleNonEvenCapacity)
{
    TilesPngWorkspace workspace{20};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    // Standard tiles.png format: 128 pixels wide (16 tiles)
    EXPECT_EQ(img.width(), 128);
    // 20 tiles / 16 tiles per row = 2 rows (ceiling) * 8 pixels per tile = 16 pixels
    EXPECT_EQ(img.height(), 16);
}

TEST(TilesPngWorkspaceTests, ExportImageShouldBeAllTransparentForEmptyWorkspace)
{
    TilesPngWorkspace workspace{16};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    // All pixels should be transparent (IndexPixel(0))
    for (std::size_t row = 0; row < img.height(); ++row) {
        for (std::size_t col = 0; col < img.width(); ++col) {
            EXPECT_EQ(img.at(row, col).index(), 0);
        }
    }
}

TEST(TilesPngWorkspaceTests, ExportImageShouldContainInsertedTiles)
{
    TilesPngWorkspace workspace{10};

    // Create and insert a tile with a specific pixel
    // Note: A tile with only pixel (0,0) set gets canonicalized to pixel (7,7)
    // So we use a tile that's already in canonical form (symmetric)
    PixelTile<IndexPixel> pixel_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        pixel_tile.set(i, IndexPixel{42});
    }
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::ignore = workspace.insert_tile(tile);

    auto img = workspace.export_image();

    // Tile 1 should be at pixel position (0, 8) since tile 0 is transparent
    // Tile 1 starts at pixel column 8 (second tile in first row)
    // Check that all pixels in the tile are 42
    EXPECT_EQ(img.at(0, 8).index(), 42);
    EXPECT_EQ(img.at(7, 15).index(), 42);
}

TEST(TilesPngWorkspaceTests, RoundTripShouldPreserveImageContents)
{
    // Create a simple image with distinct tiles using canonical-form-friendly patterns
    // We fill entire tiles with solid colors to ensure they remain canonical
    Image<IndexPixel> original{16, 8};

    // First tile (0-7, 0-7) - fill with value 1
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            original.set(row, col, IndexPixel{1});
        }
    }

    // Second tile (0-7, 8-15) - fill with value 2
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            original.set(row, col, IndexPixel{2});
        }
    }

    // Load into workspace
    TilesPngWorkspace workspace{original, 10};

    // Export back to image
    auto exported = workspace.export_image();

    // Verify the tiles are preserved - check a few pixels from each tile
    EXPECT_EQ(exported.at(0, 0).index(), 1);
    EXPECT_EQ(exported.at(4, 4).index(), 1);
    EXPECT_EQ(exported.at(7, 7).index(), 1);

    EXPECT_EQ(exported.at(0, 8).index(), 2);
    EXPECT_EQ(exported.at(4, 12).index(), 2);
    EXPECT_EQ(exported.at(7, 15).index(), 2);
}

TEST(TilesPngWorkspaceTests, RoundTripShouldPreserveComplexImage)
{
    // Create a more complex image with canonical-form-friendly tiles
    Image<IndexPixel> original{32, 16};

    // Fill different tiles with solid colors to ensure canonical forms
    // Tile at (0, 0) - fill with value 10
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            original.set(row, col, IndexPixel{10});
        }
    }

    // Tile at (0, 1) - fill with value 20
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            original.set(row, col, IndexPixel{20});
        }
    }

    // Tile at (0, 2) - fill with value 30
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 16; col < 24; ++col) {
            original.set(row, col, IndexPixel{30});
        }
    }

    // Tile at (1, 0) - fill with value 40
    for (std::size_t row = 8; row < 16; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            original.set(row, col, IndexPixel{40});
        }
    }

    // Load into workspace
    TilesPngWorkspace workspace{original, 256};

    // Export back to image
    auto exported = workspace.export_image();

    // Verify dimensions match (workspace capacity may be larger)
    EXPECT_EQ(exported.width(), 128); // Standard width

    // Verify the specific pixels are preserved (check multiple pixels per tile)
    // Note: Original image was 4 tiles wide x 2 tiles tall, but exported is 16 tiles wide
    // Tiles are laid out in row-major order in the exported image
    EXPECT_EQ(exported.at(0, 0).index(), 10); // Tile 0 at (0,0)
    EXPECT_EQ(exported.at(7, 7).index(), 10);
    EXPECT_EQ(exported.at(0, 8).index(), 20); // Tile 1 at (0,1)
    EXPECT_EQ(exported.at(7, 15).index(), 20);
    EXPECT_EQ(exported.at(0, 16).index(), 30); // Tile 2 at (0,2)
    EXPECT_EQ(exported.at(7, 23).index(), 30);
    EXPECT_EQ(exported.at(0, 32).index(), 40); // Tile 4 at (0,4) in exported layout
    EXPECT_EQ(exported.at(7, 39).index(), 40);
}

TEST(TilesPngWorkspaceTests, ExportImageShouldPlaceTilesInCorrectPositions)
{
    TilesPngWorkspace workspace{256};

    // Create and insert tiles filled with solid colors (canonical form)
    PixelTile<IndexPixel> pixel_tile1;
    for (std::size_t i = 0; i < 64; ++i) {
        pixel_tile1.set(i, IndexPixel{100});
    }
    CanonicalPixelTile<IndexPixel> tile1{pixel_tile1};
    std::ignore = workspace.insert_tile(tile1);

    // Create and insert another tile
    PixelTile<IndexPixel> pixel_tile2;
    for (std::size_t i = 0; i < 64; ++i) {
        pixel_tile2.set(i, IndexPixel{200});
    }
    CanonicalPixelTile<IndexPixel> tile2{pixel_tile2};
    std::ignore = workspace.insert_tile(tile2);

    auto img = workspace.export_image();

    // Tile 0 should be transparent
    EXPECT_EQ(img.at(0, 0).index(), 0);

    // Tile 1 should be at pixel (0, 8) - second tile in first row
    EXPECT_EQ(img.at(0, 8).index(), 100);
    EXPECT_EQ(img.at(7, 15).index(), 100); // Last pixel of tile 1

    // Tile 2 should be at pixel (0, 16) - third tile in first row
    EXPECT_EQ(img.at(0, 16).index(), 200);
    EXPECT_EQ(img.at(7, 23).index(), 200); // Last pixel of tile 2
}

// ========================================
// Export Original Tests
// ========================================

TEST(TilesPngWorkspaceTests, ExportOriginalImageShouldPreserveOriginalPixelArrangement)
{
    // Create an image with a non-canonical tile (top-left pixel set)
    Image<IndexPixel> original{8, 8};
    original.set(0, 0, IndexPixel{42});
    original.set(1, 1, IndexPixel{43});

    // Load into workspace (will be canonicalized)
    TilesPngWorkspace workspace{original, 10};

    // Export with original form restoration
    auto exported_original = workspace.export_image(ExportFlipMode::original);

    // The original pixel arrangement should be preserved
    EXPECT_EQ(exported_original.at(0, 0).index(), 42);
    EXPECT_EQ(exported_original.at(1, 1).index(), 43);

    // All other pixels should be transparent
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            if ((row == 0 && col == 0) || (row == 1 && col == 1)) {
                continue;
            }
            EXPECT_EQ(exported_original.at(row, col).index(), 0);
        }
    }
}

TEST(TilesPngWorkspaceTests, ExportOriginalVsCanonicalShouldDifferForNonCanonicalTiles)
{
    // Create an image with a tile that will be canonicalized (top-left pixel set)
    Image<IndexPixel> original{8, 8};
    original.set(0, 0, IndexPixel{99});

    // Load into workspace
    TilesPngWorkspace workspace{original, 10};

    // Export both forms
    auto exported_canonical = workspace.export_image();
    auto exported_original = workspace.export_image(ExportFlipMode::original);

    // Canonical form should have the pixel at (7,7) due to HV flip being lex-minimal
    EXPECT_EQ(exported_canonical.at(7, 7).index(), 99);
    EXPECT_EQ(exported_canonical.at(0, 0).index(), 0);

    // Original form should have the pixel at (0,0) as in the input
    EXPECT_EQ(exported_original.at(0, 0).index(), 99);
    EXPECT_EQ(exported_original.at(7, 7).index(), 0);
}

TEST(TilesPngWorkspaceTests, RoundTripWithExportOriginalShouldPreserveAllPixels)
{
    // Create an image with asymmetric tiles
    Image<IndexPixel> original{16, 8};

    // First tile with distinct pattern
    original.set(0, 0, IndexPixel{10});
    original.set(0, 1, IndexPixel{11});
    original.set(1, 0, IndexPixel{12});

    // Second tile with different pattern
    original.set(0, 8, IndexPixel{20});
    original.set(0, 9, IndexPixel{21});
    original.set(2, 8, IndexPixel{22});

    // Load into workspace
    TilesPngWorkspace workspace{original, 10};

    // Export with original form restoration
    auto exported = workspace.export_image(ExportFlipMode::original);

    // Verify all specific pixels are preserved
    EXPECT_EQ(exported.at(0, 0).index(), 10);
    EXPECT_EQ(exported.at(0, 1).index(), 11);
    EXPECT_EQ(exported.at(1, 0).index(), 12);

    EXPECT_EQ(exported.at(0, 8).index(), 20);
    EXPECT_EQ(exported.at(0, 9).index(), 21);
    EXPECT_EQ(exported.at(2, 8).index(), 22);
}

TEST(TilesPngWorkspaceTests, ExportOriginalImageShouldHandleComplexPatterns)
{
    // Create an image with a complex asymmetric pattern
    Image<IndexPixel> original{8, 8};

    // Create a diagonal gradient pattern (top-left to bottom-right)
    for (std::size_t i = 0; i < 8; ++i) {
        original.set(i, i, IndexPixel{static_cast<unsigned int>(i + 1)});
    }

    // Load into workspace
    TilesPngWorkspace workspace{original, 10};

    // Export with original form restoration
    auto exported = workspace.export_image(ExportFlipMode::original);

    // Verify the diagonal pattern is preserved
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(exported.at(i, i).index(), i + 1);
    }

    // Verify non-diagonal pixels are transparent
    EXPECT_EQ(exported.at(0, 1).index(), 0);
    EXPECT_EQ(exported.at(1, 0).index(), 0);
    EXPECT_EQ(exported.at(7, 6).index(), 0);
}

TEST(TilesPngWorkspaceTests, ExportCanonicalShouldMatchForSymmetricTiles)
{
    // Create an image with a symmetric tile (solid color)
    Image<IndexPixel> original{8, 8};
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            original.set(row, col, IndexPixel{50});
        }
    }

    // Load into workspace
    TilesPngWorkspace workspace{original, 10};

    // Export both forms
    auto exported_canonical = workspace.export_image();
    auto exported_original = workspace.export_image(ExportFlipMode::original);

    // For symmetric tiles, both exports should be identical
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            EXPECT_EQ(exported_canonical.at(row, col).index(), exported_original.at(row, col).index());
            EXPECT_EQ(exported_canonical.at(row, col).index(), 50);
        }
    }
}

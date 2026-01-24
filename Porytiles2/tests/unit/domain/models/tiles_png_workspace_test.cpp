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

    std::size_t result = workspace.insert_tile(tile);

    // Should return index 1 (first slot after reserved tile 0)
    EXPECT_EQ(result, 1);
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());
}

TEST(TilesPngWorkspaceTests, InsertTileShouldReturnZeroForTransparentTile)
{
    TilesPngWorkspace workspace{10};

    // Create a transparent tile
    PixelTile<IndexPixel> pixel_tile;
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::size_t result = workspace.insert_tile(tile);

    // Transparent tiles return 0 (the standard transparent tile index)
    EXPECT_EQ(result, 0);
}

TEST(TilesPngWorkspaceTests, InsertTileShouldPanicWhenAtCapacity)
{
    TilesPngWorkspace workspace{1};

    // Create a non-transparent tile
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    // Workspace with capacity 1 should be at capacity initially (tile 0 is reserved)
    EXPECT_TRUE(workspace.at_capacity());

    // Attempting to insert when at capacity should panic
    ASSERT_DEATH(std::ignore = workspace.insert_tile(tile), "TilesPngWorkspace is at capacity");
}

TEST(TilesPngWorkspaceTests, InsertTileShouldFillCapacityProperly)
{
    constexpr std::size_t capacity = 5;
    TilesPngWorkspace workspace{capacity};

    // Insert tiles until at capacity
    for (std::size_t i = 1; i < capacity; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};

        std::size_t result = workspace.insert_tile(tile);
        // Each tile should be inserted at its expected index
        EXPECT_EQ(result, i);
    }

    // Now should be at capacity
    EXPECT_TRUE(workspace.at_capacity());

    // Try to insert one more - should panic
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{99});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    ASSERT_DEATH(std::ignore = workspace.insert_tile(tile), "TilesPngWorkspace is at capacity");
}

TEST(TilesPngWorkspaceTests, InsertTileShouldFastForwardCursorCorrectly)
{
    TilesPngWorkspace workspace{10};

    // Insert a few non-transparent tiles
    for (std::size_t i = 0; i < 3; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i + 1});
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
        pixel_tile.set(0, 0, IndexPixel{i});
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

    std::size_t result = workspace.insert_tile(tile);
    // Should be inserted at index 2 (after the two tiles loaded from image)
    EXPECT_EQ(result, 2);

    // Verify the inserted tile via first_occurrence_of
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
        original.set(i, i, IndexPixel{i + 1});
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

// ========================================
// find_contiguous_transparent_slots Tests
// ========================================

TEST(TilesPngWorkspaceTests, FindContiguousTransparentSlotsShouldFindSlotsInFreshWorkspace)
{
    TilesPngWorkspace workspace{20};

    // Fresh workspace should have 19 contiguous transparent slots (indices 1-19, skipping tile 0)
    auto result = workspace.find_contiguous_transparent_slots(5);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1); // Should start at index 1
}

TEST(TilesPngWorkspaceTests, FindContiguousTransparentSlotsShouldFindSlotsAfterInsertions)
{
    TilesPngWorkspace workspace{20};

    // Insert some tiles at indices 1, 2, 3
    for (std::size_t i = 0; i < 3; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i + 1});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};
        std::ignore = workspace.insert_tile(tile);
    }

    // Should now find contiguous slots starting at index 4
    auto result = workspace.find_contiguous_transparent_slots(5);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 4);
}

TEST(TilesPngWorkspaceTests, FindContiguousTransparentSlotsShouldFindSlotsInFragmentedWorkspace)
{
    // Create workspace from image with scattered non-transparent tiles
    Image<IndexPixel> img{64, 8};  // 8 tiles
    img.set(0, 0, IndexPixel{1});  // Tile 0 non-transparent
    img.set(0, 16, IndexPixel{2}); // Tile 2 non-transparent
    img.set(0, 40, IndexPixel{3}); // Tile 5 non-transparent

    TilesPngWorkspace workspace{img, 20};

    // Available transparent runs: tiles 1 (len 1), tiles 3-4 (len 2), tiles 6-7 + padding (len 14)
    // Looking for 3 contiguous should find starting at tile 6
    auto result = workspace.find_contiguous_transparent_slots(3);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 6);
}

TEST(TilesPngWorkspaceTests, FindContiguousTransparentSlotsShouldReturnNulloptWhenNoSpace)
{
    // Create a nearly full workspace
    TilesPngWorkspace workspace{5};

    // Insert tiles to fill all slots except tile 0
    for (std::size_t i = 0; i < 4; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i + 1});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};
        std::ignore = workspace.insert_tile(tile);
    }

    // No contiguous space for 2 tiles
    auto result = workspace.find_contiguous_transparent_slots(2);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FindContiguousTransparentSlotsShouldHandleCountZero)
{
    TilesPngWorkspace workspace{10};

    // Count 0 is trivially satisfied
    auto result = workspace.find_contiguous_transparent_slots(0);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindContiguousTransparentSlotsShouldHandleExactCapacityMinus1)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    // Looking for capacity-1 tiles (all except tile 0)
    auto result = workspace.find_contiguous_transparent_slots(capacity - 1);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindContiguousTransparentSlotsShouldReturnNulloptWhenRequestExceedsCapacity)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    // Looking for more tiles than available (capacity - 1 is max since tile 0 is reserved)
    auto result = workspace.find_contiguous_transparent_slots(capacity);
    EXPECT_FALSE(result.has_value());
}

// ========================================
// find_existing_contiguous_tiles Tests
// ========================================

TEST(TilesPngWorkspaceTests, FindExistingContiguousTilesShouldFindExistingSequence)
{
    // Create workspace with known tile sequence at indices 1, 2, 3
    Image<IndexPixel> img{32, 8}; // 4 tiles
    // Tile 0: transparent
    // Tile 1: pattern A (filled with 10)
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{10});
        }
    }
    // Tile 2: pattern B (filled with 20)
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 16; col < 24; ++col) {
            img.set(row, col, IndexPixel{20});
        }
    }
    // Tile 3: pattern C (filled with 30)
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 24; col < 32; ++col) {
            img.set(row, col, IndexPixel{30});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Build the same sequence of tiles
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    for (std::size_t val : {10, 20, 30}) {
        PixelTile<IndexPixel> pixel_tile;
        for (std::size_t i = 0; i < 64; ++i) {
            pixel_tile.set(i, IndexPixel{val});
        }
        tiles_to_find.emplace_back(pixel_tile);
    }

    auto result = workspace.find_existing_contiguous_tiles(tiles_to_find);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1); // Should find at index 1
}

TEST(TilesPngWorkspaceTests, FindExistingContiguousTilesShouldReturnNulloptWhenScattered)
{
    // Create workspace with tiles at non-contiguous positions
    Image<IndexPixel> img{48, 8}; // 6 tiles
    // Tile 1: pattern A
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{10});
        }
    }
    // Tile 2: transparent (gap)
    // Tile 3: pattern B
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 24; col < 32; ++col) {
            img.set(row, col, IndexPixel{20});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Try to find A followed by B (not contiguous in workspace)
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    PixelTile<IndexPixel> tile_a;
    for (std::size_t i = 0; i < 64; ++i) {
        tile_a.set(i, IndexPixel{10});
    }
    tiles_to_find.emplace_back(tile_a);

    PixelTile<IndexPixel> tile_b;
    for (std::size_t i = 0; i < 64; ++i) {
        tile_b.set(i, IndexPixel{20});
    }
    tiles_to_find.emplace_back(tile_b);

    auto result = workspace.find_existing_contiguous_tiles(tiles_to_find);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FindExistingContiguousTilesShouldReturnNulloptWhenNotExists)
{
    TilesPngWorkspace workspace{20};

    // Insert some tiles
    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{5});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};
    std::ignore = workspace.insert_tile(tile);

    // Search for a different tile
    PixelTile<IndexPixel> different_tile;
    different_tile.set(0, 0, IndexPixel{99});
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    tiles_to_find.emplace_back(different_tile);

    auto result = workspace.find_existing_contiguous_tiles(tiles_to_find);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FindExistingContiguousTilesShouldHandleEmptySequence)
{
    TilesPngWorkspace workspace{10};

    std::vector<CanonicalPixelTile<IndexPixel>> empty_tiles;

    auto result = workspace.find_existing_contiguous_tiles(empty_tiles);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindExistingContiguousTilesShouldMatchCanonicalForms)
{
    // Test that flipped variants match their canonical counterparts
    TilesPngWorkspace workspace{20};

    // Insert a tile that will be canonicalized
    PixelTile<IndexPixel> original_tile;
    original_tile.set(0, 0, IndexPixel{42}); // Top-left pixel set

    CanonicalPixelTile<IndexPixel> canonical_tile{original_tile};
    std::ignore = workspace.insert_tile(canonical_tile);

    // Search for the same canonical tile
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    tiles_to_find.push_back(canonical_tile);

    auto result = workspace.find_existing_contiguous_tiles(tiles_to_find);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

// ========================================
// place_tiles_at Tests
// ========================================

TEST(TilesPngWorkspaceTests, PlaceTilesAtShouldPlaceTilesAtSpecifiedPosition)
{
    TilesPngWorkspace workspace{20};

    // Create tiles to place
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_place;
    for (std::size_t val : {10, 20, 30}) {
        PixelTile<IndexPixel> pixel_tile;
        for (std::size_t i = 0; i < 64; ++i) {
            pixel_tile.set(i, IndexPixel{val});
        }
        tiles_to_place.emplace_back(pixel_tile);
    }

    // Place at position 5
    workspace.place_tiles_at(5, tiles_to_place);

    // Verify tiles are at positions 5, 6, 7
    EXPECT_FALSE(workspace.tile_at(5).is_transparent());
    EXPECT_FALSE(workspace.tile_at(6).is_transparent());
    EXPECT_FALSE(workspace.tile_at(7).is_transparent());

    // Verify positions before and after are still transparent
    EXPECT_TRUE(workspace.tile_at(4).is_transparent());
    EXPECT_TRUE(workspace.tile_at(8).is_transparent());
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtShouldUpdateCanonicalFormsMap)
{
    TilesPngWorkspace workspace{20};

    // Create a tile
    PixelTile<IndexPixel> pixel_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        pixel_tile.set(i, IndexPixel{42});
    }
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_place;
    tiles_to_place.push_back(tile);

    // Place at position 10
    workspace.place_tiles_at(10, tiles_to_place);

    // Should be findable via first_occurrence_of
    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 10);
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtShouldPanicWhenPositionNotTransparent)
{
    TilesPngWorkspace workspace{20};

    // Insert a tile at position 5
    PixelTile<IndexPixel> blocking_tile;
    blocking_tile.set(0, 0, IndexPixel{99});
    CanonicalPixelTile<IndexPixel> blocking{blocking_tile};

    // Insert tiles at 1, 2, 3, 4 first to move cursor to 5
    for (std::size_t i = 0; i < 4; ++i) {
        PixelTile<IndexPixel> pt;
        pt.set(0, 0, IndexPixel{i + 1});
        CanonicalPixelTile<IndexPixel> t{pt};
        std::ignore = workspace.insert_tile(t);
    }

    // Now position 5 has a tile, trying to place there should panic
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_place;
    PixelTile<IndexPixel> new_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        new_tile.set(i, IndexPixel{50});
    }
    tiles_to_place.emplace_back(new_tile);

    // This should panic because we're trying to overwrite position 1 (not transparent)
    ASSERT_DEATH(workspace.place_tiles_at(1, tiles_to_place), "is not transparent");
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtShouldAdvanceCursorPastPlacedTiles)
{
    TilesPngWorkspace workspace{20};

    // Cursor starts at 1
    // Place tiles at 1, 2, 3
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_place;
    for (std::size_t val : {10, 20, 30}) {
        PixelTile<IndexPixel> pixel_tile;
        for (std::size_t i = 0; i < 64; ++i) {
            pixel_tile.set(i, IndexPixel{val});
        }
        tiles_to_place.emplace_back(pixel_tile);
    }

    workspace.place_tiles_at(1, tiles_to_place);

    // Now insert another tile - it should go to position 4
    PixelTile<IndexPixel> new_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        new_tile.set(i, IndexPixel{40});
    }
    CanonicalPixelTile<IndexPixel> tile{new_tile};

    std::size_t inserted_at = workspace.insert_tile(tile);
    EXPECT_EQ(inserted_at, 4);
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtShouldPanicWhenExceedsCapacity)
{
    TilesPngWorkspace workspace{5};

    // Try to place 3 tiles starting at position 4 (would need positions 4, 5, 6 but capacity is 5)
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_place;
    for (std::size_t i = 0; i < 3; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        for (std::size_t j = 0; j < 64; ++j) {
            pixel_tile.set(j, IndexPixel{i + 1});
        }
        tiles_to_place.emplace_back(pixel_tile);
    }

    ASSERT_DEATH(workspace.place_tiles_at(4, tiles_to_place), "exceeds capacity");
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtShouldHandleEmptyTileVector)
{
    TilesPngWorkspace workspace{10};

    std::vector<CanonicalPixelTile<IndexPixel>> empty_tiles;

    // Should not panic, just do nothing
    workspace.place_tiles_at(5, empty_tiles);

    // Workspace should be unchanged
    EXPECT_TRUE(workspace.tile_at(5).is_transparent());
}

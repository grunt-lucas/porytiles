#include "gtest/gtest.h"

#include "porytiles/domain/models/tiles_png_workspace.hpp"

using namespace porytiles;

TEST(TilesPngWorkspaceTests, CtorCapacityInit)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    EXPECT_EQ(workspace.capacity(), capacity);
}

TEST(TilesPngWorkspaceTests, CtorAllTilesTransparent)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    for (std::size_t i = 0; i < capacity; ++i) {
        EXPECT_TRUE(workspace.tile_at(i).is_transparent());
    }
}

TEST(TilesPngWorkspaceTests, CtorNotAtCapacity)
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

TEST(TilesPngWorkspaceTests, CtorFromImageLoadsValid)
{
    Image<IndexPixel> img{8, 8};
    img.set(0, 0, IndexPixel{1});
    img.set(1, 1, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    EXPECT_EQ(workspace.capacity(), 10);
    EXPECT_FALSE(workspace.at_capacity());

    EXPECT_FALSE(workspace.tile_at(0).is_transparent());

    for (std::size_t i = 1; i < 10; ++i) {
        EXPECT_TRUE(workspace.tile_at(i).is_transparent());
    }
}

TEST(TilesPngWorkspaceTests, CtorFromImageExtractsMultipleTiles)
{
    Image<IndexPixel> img{16, 8};
    img.set(0, 0, IndexPixel{1});
    img.set(0, 8, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    EXPECT_EQ(workspace.capacity(), 10);

    EXPECT_FALSE(workspace.tile_at(0).is_transparent());
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());
}

TEST(TilesPngWorkspaceTests, CtorFromImageAllTransparent)
{
    Image<IndexPixel> img{16, 16};

    TilesPngWorkspace workspace{img, 10};

    EXPECT_EQ(workspace.capacity(), 10);

    for (std::size_t i = 0; i < 10; ++i) {
        EXPECT_TRUE(workspace.tile_at(i).is_transparent());
    }

    EXPECT_FALSE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, CtorFromImagePanicsInvalidWidth)
{
    Image<IndexPixel> img{7, 8};

    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 10), "Image dimensions must be a multiple of 8");
}

TEST(TilesPngWorkspaceTests, CtorFromImagePanicsInvalidHeight)
{
    Image<IndexPixel> img{8, 7};

    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 10), "Image dimensions must be a multiple of 8");
}

TEST(TilesPngWorkspaceTests, CtorFromImagePanicsInvalidBoth)
{
    Image<IndexPixel> img{15, 15};

    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 10), "Image dimensions must be a multiple of 8");
}

TEST(TilesPngWorkspaceTests, CtorFromImagePanicsExceedsCapacity)
{
    Image<IndexPixel> img{16, 16};
    ASSERT_DEATH(std::ignore = TilesPngWorkspace(img, 3), "Image contains 4 tiles but capacity is only 3");
}

TEST(TilesPngWorkspaceTests, CtorFromImageExactCapacity)
{
    Image<IndexPixel> img{16, 16};
    img.set(0, 0, IndexPixel{1});

    TilesPngWorkspace workspace{img, 4};

    EXPECT_EQ(workspace.capacity(), 4);
}

TEST(TilesPngWorkspaceTests, InsertTileNonTransparent)
{
    TilesPngWorkspace workspace{10};

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::size_t result = workspace.insert_tile(tile);

    EXPECT_EQ(result, 1);
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());
}

TEST(TilesPngWorkspaceTests, InsertTileTransparentReturnsZero)
{
    TilesPngWorkspace workspace{10};

    PixelTile<IndexPixel> pixel_tile;
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::size_t result = workspace.insert_tile(tile);

    EXPECT_EQ(result, 0);
}

TEST(TilesPngWorkspaceTests, InsertTilePanicsAtCapacity)
{
    TilesPngWorkspace workspace{1};

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    EXPECT_TRUE(workspace.at_capacity());
    ASSERT_DEATH(std::ignore = workspace.insert_tile(tile), "TilesPngWorkspace is at capacity");
}

TEST(TilesPngWorkspaceTests, InsertTileFillsCapacity)
{
    constexpr std::size_t capacity = 5;
    TilesPngWorkspace workspace{capacity};

    for (std::size_t i = 1; i < capacity; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};

        std::size_t result = workspace.insert_tile(tile);
        EXPECT_EQ(result, i);
    }

    EXPECT_TRUE(workspace.at_capacity());

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{99});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    ASSERT_DEATH(std::ignore = workspace.insert_tile(tile), "TilesPngWorkspace is at capacity");
}

TEST(TilesPngWorkspaceTests, InsertTileFastForwardsCursor)
{
    TilesPngWorkspace workspace{10};

    for (std::size_t i = 0; i < 3; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i + 1});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};

        std::ignore = workspace.insert_tile(tile);
    }

    EXPECT_FALSE(workspace.at_capacity());

    EXPECT_FALSE(workspace.tile_at(1).is_transparent());
    EXPECT_FALSE(workspace.tile_at(2).is_transparent());
    EXPECT_FALSE(workspace.tile_at(3).is_transparent());
    EXPECT_TRUE(workspace.tile_at(4).is_transparent());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceFindsInserted)
{
    TilesPngWorkspace workspace{10};

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{5});
    pixel_tile.set(1, 1, IndexPixel{7});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::ignore = workspace.insert_tile(tile);

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 1);
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceNulloptForMissing)
{
    TilesPngWorkspace workspace{10};

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{99});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_FALSE(occurrence.has_value());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceNulloptForTransparent)
{
    TilesPngWorkspace workspace{10};

    PixelTile<IndexPixel> pixel_tile;
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_FALSE(occurrence.has_value());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceFromImage)
{
    Image<IndexPixel> img{8, 8};
    img.set(0, 0, IndexPixel{1});
    img.set(1, 1, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    pixel_tile.set(1, 1, IndexPixel{2});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 0);
}

TEST(TilesPngWorkspaceTests, TileAtReturnsCorrectTile)
{
    TilesPngWorkspace workspace{10};

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{1});
    pixel_tile.set(1, 1, IndexPixel{2});
    pixel_tile.set(2, 2, IndexPixel{3});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::ignore = workspace.insert_tile(tile);

    auto retrieved_tile = workspace.tile_at(1);
    EXPECT_FALSE(retrieved_tile.is_transparent());
    EXPECT_EQ(retrieved_tile, tile);
}

TEST(TilesPngWorkspaceTests, TileAtPanicsOutOfBounds)
{
    TilesPngWorkspace workspace{10};

    EXPECT_DEATH(std::ignore = workspace.tile_at(10), "index 10 >= size 10");
}

TEST(TilesPngWorkspaceTests, TileAtAllValidIndices)
{
    constexpr std::size_t capacity = 5;
    TilesPngWorkspace workspace{capacity};

    for (std::size_t i = 0; i < capacity; ++i) {
        auto tile = workspace.tile_at(i);
        EXPECT_TRUE(tile.is_transparent());
    }
}

TEST(TilesPngWorkspaceTests, AtCapacityFalseInitially)
{
    TilesPngWorkspace workspace{10};
    EXPECT_FALSE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, AtCapacityTrueWhenFull)
{
    constexpr std::size_t capacity = 3;
    TilesPngWorkspace workspace{capacity};

    for (std::size_t i = 1; i < capacity; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};
        std::ignore = workspace.insert_tile(tile);
    }

    EXPECT_TRUE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, AtCapacityTrueForCapacityOne)
{
    TilesPngWorkspace workspace{1};
    EXPECT_TRUE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, ComplexWorkflow)
{
    Image<IndexPixel> img{16, 8};
    img.set(0, 0, IndexPixel{1});
    img.set(0, 8, IndexPixel{2});

    TilesPngWorkspace workspace{img, 10};

    EXPECT_FALSE(workspace.tile_at(0).is_transparent());
    EXPECT_FALSE(workspace.tile_at(1).is_transparent());

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{3});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    std::size_t result = workspace.insert_tile(tile);
    EXPECT_EQ(result, 2);

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 2);

    EXPECT_FALSE(workspace.at_capacity());
}

TEST(TilesPngWorkspaceTests, DeduplicatesTilesFromImage)
{
    Image<IndexPixel> img{16, 8};
    img.set(0, 0, IndexPixel{5});
    img.set(0, 8, IndexPixel{5});

    TilesPngWorkspace workspace{img, 10};

    PixelTile<IndexPixel> pixel_tile;
    pixel_tile.set(0, 0, IndexPixel{5});
    CanonicalPixelTile<IndexPixel> tile{pixel_tile};

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 0);
}

TEST(TilesPngWorkspaceTests, ExportImageDimensions)
{
    TilesPngWorkspace workspace{256};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    // Standard tiles.png format: 128 pixels wide (16 tiles)
    EXPECT_EQ(img.width(), 128);
    // 256 tiles / 16 tiles per row = 16 rows * 8 pixels per tile = 128 pixels
    EXPECT_EQ(img.height(), 128);
}

TEST(TilesPngWorkspaceTests, ExportImageNonSquareHeight)
{
    TilesPngWorkspace workspace{32};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    // Standard tiles.png format: 128 pixels wide (16 tiles)
    EXPECT_EQ(img.width(), 128);
    // 32 tiles / 16 tiles per row = 2 rows * 8 pixels per tile = 16 pixels
    EXPECT_EQ(img.height(), 16);
}

TEST(TilesPngWorkspaceTests, ExportImageNonEvenCapacity)
{
    TilesPngWorkspace workspace{20};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    // Standard tiles.png format: 128 pixels wide (16 tiles)
    EXPECT_EQ(img.width(), 128);
    // 20 tiles / 16 tiles per row = 2 rows (ceiling) * 8 pixels per tile = 16 pixels
    EXPECT_EQ(img.height(), 16);
}

TEST(TilesPngWorkspaceTests, ExportImageEmptyIsTransparent)
{
    TilesPngWorkspace workspace{16};

    auto img = workspace.export_image(ExportFlipMode::original, ExportTrimMode::include_trailing_transparent);

    for (std::size_t row = 0; row < img.height(); ++row) {
        for (std::size_t col = 0; col < img.width(); ++col) {
            EXPECT_EQ(img.at(row, col).index(), 0);
        }
    }
}

TEST(TilesPngWorkspaceTests, ExportImageContainsInserted)
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

    EXPECT_EQ(img.at(0, 8).index(), 42);
    EXPECT_EQ(img.at(7, 15).index(), 42);
}

TEST(TilesPngWorkspaceTests, RoundTripPreservesContents)
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

TEST(TilesPngWorkspaceTests, RoundTripPreservesComplex)
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

    TilesPngWorkspace workspace{original, 256};

    auto exported = workspace.export_image();

    EXPECT_EQ(exported.width(), 128);

    // Original image was 4 tiles wide x 2 tiles tall, but exported is 16 tiles wide
    EXPECT_EQ(exported.at(0, 0).index(), 10); // Tile 0 at (0,0)
    EXPECT_EQ(exported.at(7, 7).index(), 10);
    EXPECT_EQ(exported.at(0, 8).index(), 20); // Tile 1 at (0,1)
    EXPECT_EQ(exported.at(7, 15).index(), 20);
    EXPECT_EQ(exported.at(0, 16).index(), 30); // Tile 2 at (0,2)
    EXPECT_EQ(exported.at(7, 23).index(), 30);
    EXPECT_EQ(exported.at(0, 32).index(), 40); // Tile 4 at (0,4) in exported layout
    EXPECT_EQ(exported.at(7, 39).index(), 40);
}

TEST(TilesPngWorkspaceTests, ExportImageTilePositions)
{
    TilesPngWorkspace workspace{256};

    PixelTile<IndexPixel> pixel_tile1;
    for (std::size_t i = 0; i < 64; ++i) {
        pixel_tile1.set(i, IndexPixel{100});
    }
    CanonicalPixelTile<IndexPixel> tile1{pixel_tile1};
    std::ignore = workspace.insert_tile(tile1);

    PixelTile<IndexPixel> pixel_tile2;
    for (std::size_t i = 0; i < 64; ++i) {
        pixel_tile2.set(i, IndexPixel{200});
    }
    CanonicalPixelTile<IndexPixel> tile2{pixel_tile2};
    std::ignore = workspace.insert_tile(tile2);

    auto img = workspace.export_image();

    EXPECT_EQ(img.at(0, 0).index(), 0);

    EXPECT_EQ(img.at(0, 8).index(), 100);
    EXPECT_EQ(img.at(7, 15).index(), 100);

    EXPECT_EQ(img.at(0, 16).index(), 200);
    EXPECT_EQ(img.at(7, 23).index(), 200);
}

TEST(TilesPngWorkspaceTests, ExportOriginalPreservesPixels)
{
    Image<IndexPixel> original{8, 8};
    original.set(0, 0, IndexPixel{42});
    original.set(1, 1, IndexPixel{43});

    TilesPngWorkspace workspace{original, 10};

    auto exported_original = workspace.export_image(ExportFlipMode::original);
    EXPECT_EQ(exported_original.at(0, 0).index(), 42);
    EXPECT_EQ(exported_original.at(1, 1).index(), 43);

    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            if ((row == 0 && col == 0) || (row == 1 && col == 1)) {
                continue;
            }
            EXPECT_EQ(exported_original.at(row, col).index(), 0);
        }
    }
}

TEST(TilesPngWorkspaceTests, ExportOriginalVsCanonicalDiffers)
{
    Image<IndexPixel> original{8, 8};
    original.set(0, 0, IndexPixel{99});

    TilesPngWorkspace workspace{original, 10};

    auto exported_canonical = workspace.export_image();
    auto exported_original = workspace.export_image(ExportFlipMode::original);

    // Canonical form should have the pixel at (7,7) due to HV flip being lex-minimal
    EXPECT_EQ(exported_canonical.at(7, 7).index(), 99);
    EXPECT_EQ(exported_canonical.at(0, 0).index(), 0);

    // Original form should have the pixel at (0,0) as in the input
    EXPECT_EQ(exported_original.at(0, 0).index(), 99);
    EXPECT_EQ(exported_original.at(7, 7).index(), 0);
}

TEST(TilesPngWorkspaceTests, RoundTripOriginalPreservesPixels)
{
    Image<IndexPixel> original{16, 8};

    original.set(0, 0, IndexPixel{10});
    original.set(0, 1, IndexPixel{11});
    original.set(1, 0, IndexPixel{12});

    original.set(0, 8, IndexPixel{20});
    original.set(0, 9, IndexPixel{21});
    original.set(2, 8, IndexPixel{22});

    TilesPngWorkspace workspace{original, 10};

    auto exported = workspace.export_image(ExportFlipMode::original);
    EXPECT_EQ(exported.at(0, 0).index(), 10);
    EXPECT_EQ(exported.at(0, 1).index(), 11);
    EXPECT_EQ(exported.at(1, 0).index(), 12);

    EXPECT_EQ(exported.at(0, 8).index(), 20);
    EXPECT_EQ(exported.at(0, 9).index(), 21);
    EXPECT_EQ(exported.at(2, 8).index(), 22);
}

TEST(TilesPngWorkspaceTests, ExportOriginalComplexPatterns)
{
    Image<IndexPixel> original{8, 8};

    for (std::size_t i = 0; i < 8; ++i) {
        original.set(i, i, IndexPixel{i + 1});
    }

    TilesPngWorkspace workspace{original, 10};

    auto exported = workspace.export_image(ExportFlipMode::original);

    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(exported.at(i, i).index(), i + 1);
    }

    EXPECT_EQ(exported.at(0, 1).index(), 0);
    EXPECT_EQ(exported.at(1, 0).index(), 0);
    EXPECT_EQ(exported.at(7, 6).index(), 0);
}

TEST(TilesPngWorkspaceTests, ExportCanonicalMatchesSymmetric)
{
    Image<IndexPixel> original{8, 8};
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            original.set(row, col, IndexPixel{50});
        }
    }

    TilesPngWorkspace workspace{original, 10};

    auto exported_canonical = workspace.export_image();
    auto exported_original = workspace.export_image(ExportFlipMode::original);
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            EXPECT_EQ(exported_canonical.at(row, col).index(), exported_original.at(row, col).index());
            EXPECT_EQ(exported_canonical.at(row, col).index(), 50);
        }
    }
}

TEST(TilesPngWorkspaceTests, FindContiguousSlotsFresh)
{
    TilesPngWorkspace workspace{20};

    // Fresh workspace should have 19 contiguous transparent slots (indices 1-19, skipping tile 0)
    auto result = workspace.find_contiguous_transparent_slots(5);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1); // Should start at index 1
}

TEST(TilesPngWorkspaceTests, FindContiguousSlotsAfterInsertions)
{
    TilesPngWorkspace workspace{20};

    // Insert some tiles at indices 1, 2, 3
    for (std::size_t i = 0; i < 3; ++i) {
        PixelTile<IndexPixel> pixel_tile;
        pixel_tile.set(0, 0, IndexPixel{i + 1});
        CanonicalPixelTile<IndexPixel> tile{pixel_tile};
        std::ignore = workspace.insert_tile(tile);
    }

    auto result = workspace.find_contiguous_transparent_slots(5);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 4);
}

TEST(TilesPngWorkspaceTests, FindContiguousSlotsFragmented)
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

TEST(TilesPngWorkspaceTests, FindContiguousSlotsNulloptWhenFull)
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

    auto result = workspace.find_contiguous_transparent_slots(2);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FindContiguousSlotsCountZero)
{
    TilesPngWorkspace workspace{10};

    // Count 0 is trivially satisfied
    auto result = workspace.find_contiguous_transparent_slots(0);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindContiguousSlotsExactCapacity)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    // Looking for capacity-1 tiles (all except tile 0)
    auto result = workspace.find_contiguous_transparent_slots(capacity - 1);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindContiguousSlotsExceedsCapacity)
{
    constexpr std::size_t capacity = 10;
    TilesPngWorkspace workspace{capacity};

    // Looking for more tiles than available (capacity - 1 is max since tile 0 is reserved)
    auto result = workspace.find_contiguous_transparent_slots(capacity);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtPosition)
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

    EXPECT_FALSE(workspace.tile_at(5).is_transparent());
    EXPECT_FALSE(workspace.tile_at(6).is_transparent());
    EXPECT_FALSE(workspace.tile_at(7).is_transparent());

    EXPECT_TRUE(workspace.tile_at(4).is_transparent());
    EXPECT_TRUE(workspace.tile_at(8).is_transparent());
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtUpdatesCanonical)
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

    auto occurrence = workspace.first_occurrence_of(tile);
    EXPECT_TRUE(occurrence.has_value());
    EXPECT_EQ(occurrence.value(), 10);
}

TEST(TilesPngWorkspaceTests, PlaceTilesAtPanicsNonTransparent)
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

TEST(TilesPngWorkspaceTests, PlaceTilesAtAdvancesCursor)
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

TEST(TilesPngWorkspaceTests, PlaceTilesAtPanicsExceedsCapacity)
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

TEST(TilesPngWorkspaceTests, PlaceTilesAtEmptyVector)
{
    TilesPngWorkspace workspace{10};

    std::vector<CanonicalPixelTile<IndexPixel>> empty_tiles;

    // Should not panic, just do nothing
    workspace.place_tiles_at(5, empty_tiles);

    EXPECT_TRUE(workspace.tile_at(5).is_transparent());
}

TEST(TilesPngWorkspaceTests, FindByColorMatchesDuplicateIndices)
{
    // This is the core bug fix test case:
    // Palette has duplicate colors at slots 7 and 14
    // Workspace tile uses slot 14, computed tile uses slot 7
    // They should still match because palette[7] == palette[14]

    // Set up a palette with duplicate colors
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 255});        // Transparent slot (unused for color matching)
    palette.set(7, Rgba32{100, 150, 200, 255});  // Color at slot 7
    palette.set(14, Rgba32{100, 150, 200, 255}); // Same color at slot 14

    // Create a tile in the workspace using index 14
    PixelTile<IndexPixel> workspace_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        workspace_tile.set(i, IndexPixel{14});
    }

    // Create workspace with this tile at index 1
    Image<IndexPixel> img{16, 8}; // 2 tiles
    // Tile 0: transparent (all zeros by default)
    // Tile 1: filled with index 14
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{14});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search tile using index 7 (different index, same color)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{7});
    }

    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    tiles_to_find.emplace_back(search_tile);

    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;
    palettes.push_back(&palette);

    // Should find the tile despite different indices, because colors match
    auto result = workspace.find_existing_contiguous_tiles_by_color(tiles_to_find, palettes);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindByColorRejectsDifferentColors)
{
    // Set up a palette with different colors at slots 7 and 14
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 255});
    palette.set(7, Rgba32{100, 150, 200, 255}); // Color A at slot 7
    palette.set(14, Rgba32{200, 100, 50, 255}); // Different color B at slot 14

    // Create workspace with tile using index 14
    Image<IndexPixel> img{16, 8}; // 2 tiles
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{14});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search tile using index 7 (different index, different color)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{7});
    }

    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    tiles_to_find.emplace_back(search_tile);

    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;
    palettes.push_back(&palette);

    auto result = workspace.find_existing_contiguous_tiles_by_color(tiles_to_find, palettes);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FindByColorTransparentPixels)
{
    // Transparency (index 0) should only match transparency, not other indices
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0}); // Transparent
    palette.set(1, Rgba32{100, 150, 200, 255});

    // Create workspace with tile that has some transparent pixels
    Image<IndexPixel> img{16, 8}; // 2 tiles
    // Tile 1: alternating transparent and non-transparent
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            if ((row + col) % 2 == 0) {
                img.set(row, col, IndexPixel{0}); // Transparent
            }
            else {
                img.set(row, col, IndexPixel{1}); // Non-transparent
            }
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search tile with same pattern
    PixelTile<IndexPixel> search_tile;
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            if ((row + col) % 2 == 0) {
                search_tile.set(row, col, IndexPixel{0});
            }
            else {
                search_tile.set(row, col, IndexPixel{1});
            }
        }
    }

    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    tiles_to_find.emplace_back(search_tile);

    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;
    palettes.push_back(&palette);

    auto result = workspace.find_existing_contiguous_tiles_by_color(tiles_to_find, palettes);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindByColorTransparencyMismatch)
{
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0});
    palette.set(1, Rgba32{100, 150, 200, 255});

    // Create workspace with all non-transparent tile
    Image<IndexPixel> img{16, 8}; // 2 tiles
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{1});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search tile with some transparent pixels
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{static_cast<std::size_t>(i % 2 == 0 ? 0 : 1)}); // Half transparent
    }

    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    tiles_to_find.emplace_back(search_tile);

    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;
    palettes.push_back(&palette);

    auto result = workspace.find_existing_contiguous_tiles_by_color(tiles_to_find, palettes);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FindByColorContiguousSequence)
{
    // Test finding a multi-tile contiguous sequence
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0});
    palette.set(1, Rgba32{10, 10, 10, 255});
    palette.set(2, Rgba32{20, 20, 20, 255});
    palette.set(3, Rgba32{30, 30, 30, 255});

    // Create workspace with 3 contiguous tiles at indices 1, 2, 3
    Image<IndexPixel> img{32, 8}; // 4 tiles
    // Tile 1: filled with index 1
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{1});
        }
    }
    // Tile 2: filled with index 2
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 16; col < 24; ++col) {
            img.set(row, col, IndexPixel{2});
        }
    }
    // Tile 3: filled with index 3
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 24; col < 32; ++col) {
            img.set(row, col, IndexPixel{3});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search sequence
    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;

    for (std::uint8_t idx : {1, 2, 3}) {
        PixelTile<IndexPixel> tile;
        for (std::size_t i = 0; i < 64; ++i) {
            tile.set(i, IndexPixel{idx});
        }
        tiles_to_find.emplace_back(tile);
        palettes.push_back(&palette);
    }

    auto result = workspace.find_existing_contiguous_tiles_by_color(tiles_to_find, palettes);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindByColorNulloptWhenMissing)
{
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0});
    palette.set(1, Rgba32{10, 10, 10, 255});
    palette.set(5, Rgba32{50, 50, 50, 255});

    // Create workspace with tile using index 1
    Image<IndexPixel> img{16, 8}; // 2 tiles
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{1});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Search for a different tile (index 5)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{5});
    }

    std::vector<CanonicalPixelTile<IndexPixel>> tiles_to_find;
    tiles_to_find.emplace_back(search_tile);

    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;
    palettes.push_back(&palette);

    auto result = workspace.find_existing_contiguous_tiles_by_color(tiles_to_find, palettes);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FindByColorEmptySequence)
{
    TilesPngWorkspace workspace{10};

    std::vector<CanonicalPixelTile<IndexPixel>> empty_tiles;
    std::vector<const Palette<Rgba32, palette::max_size> *> empty_palettes;

    auto result = workspace.find_existing_contiguous_tiles_by_color(empty_tiles, empty_palettes);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FindByColorPanicsMismatchedSizes)
{
    TilesPngWorkspace workspace{10};

    PixelTile<IndexPixel> tile;
    tile.set(0, 0, IndexPixel{1});

    std::vector<CanonicalPixelTile<IndexPixel>> tiles;
    tiles.emplace_back(tile);

    std::vector<const Palette<Rgba32, palette::max_size> *> palettes;
    // Intentionally leave palettes empty to cause mismatch

    ASSERT_DEATH(
        std::ignore = workspace.find_existing_contiguous_tiles_by_color(tiles, palettes),
        "tiles and palettes vectors must have the same size");
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceByColorMatchesDuplicates)
{
    // This is the core bug fix test case:
    // Palette has duplicate colors at slots 7 and 14
    // Workspace tile uses slot 14, computed tile uses slot 7
    // They should still match because palette[7] == palette[14]

    // Set up a palette with duplicate colors
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 255});        // Transparent slot (unused for color matching)
    palette.set(7, Rgba32{100, 150, 200, 255});  // Color at slot 7
    palette.set(14, Rgba32{100, 150, 200, 255}); // Same color at slot 14

    // Create workspace with tile using index 14
    Image<IndexPixel> img{16, 8}; // 2 tiles
    // Tile 0: transparent (all zeros by default)
    // Tile 1: filled with index 14
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{14});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search tile using index 7 (different index, same color)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{7});
    }
    CanonicalPixelTile<IndexPixel> canonical_search_tile{search_tile};

    // Should find the tile despite different indices, because colors match
    auto result = workspace.first_occurrence_of_by_color(canonical_search_tile, palette);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceByColorRejectsDifferent)
{
    // Set up a palette with different colors at slots 7 and 14
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 255});
    palette.set(7, Rgba32{100, 150, 200, 255}); // Color A at slot 7
    palette.set(14, Rgba32{200, 100, 50, 255}); // Different color B at slot 14

    // Create workspace with tile using index 14
    Image<IndexPixel> img{16, 8}; // 2 tiles
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{14});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search tile using index 7 (different index, different color)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{7});
    }
    CanonicalPixelTile<IndexPixel> canonical_search_tile{search_tile};

    auto result = workspace.first_occurrence_of_by_color(canonical_search_tile, palette);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceByColorNulloptTransparent)
{
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0});
    palette.set(1, Rgba32{100, 150, 200, 255});

    TilesPngWorkspace workspace{20};

    // Insert a non-transparent tile
    PixelTile<IndexPixel> workspace_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        workspace_tile.set(i, IndexPixel{1});
    }
    CanonicalPixelTile<IndexPixel> canonical_workspace_tile{workspace_tile};
    std::ignore = workspace.insert_tile(canonical_workspace_tile);

    // Create a transparent search tile (all index 0)
    PixelTile<IndexPixel> transparent_tile;
    CanonicalPixelTile<IndexPixel> canonical_transparent_tile{transparent_tile};

    // Transparent tiles should return nullopt
    auto result = workspace.first_occurrence_of_by_color(canonical_transparent_tile, palette);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceByColorNulloptMissing)
{
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0});
    palette.set(1, Rgba32{10, 10, 10, 255});
    palette.set(5, Rgba32{50, 50, 50, 255});

    // Create workspace with tile using index 1
    Image<IndexPixel> img{16, 8}; // 2 tiles
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{1});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Search for a different tile (index 5)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{5});
    }
    CanonicalPixelTile<IndexPixel> canonical_search_tile{search_tile};

    auto result = workspace.first_occurrence_of_by_color(canonical_search_tile, palette);
    EXPECT_FALSE(result.has_value());
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceByColorExactMatch)
{
    // Even with exact indices (no duplicates), the color match should work
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0});
    palette.set(5, Rgba32{50, 100, 150, 255});

    // Create workspace with tile using index 5
    Image<IndexPixel> img{16, 8}; // 2 tiles
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{5});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Search for same tile (index 5)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        search_tile.set(i, IndexPixel{5});
    }
    CanonicalPixelTile<IndexPixel> canonical_search_tile{search_tile};

    auto result = workspace.first_occurrence_of_by_color(canonical_search_tile, palette);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 1);
}

TEST(TilesPngWorkspaceTests, FirstOccurrenceByColorTransparencyMismatch)
{
    // Index 0 is special (transparent) and should only match other index 0 pixels
    Palette<Rgba32, palette::max_size> palette{};
    palette.set(0, Rgba32{0, 0, 0, 0}); // Transparent
    palette.set(1, Rgba32{100, 150, 200, 255});

    // Create workspace with tile that has all non-transparent pixels
    Image<IndexPixel> img{16, 8}; // 2 tiles
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 8; col < 16; ++col) {
            img.set(row, col, IndexPixel{1});
        }
    }

    TilesPngWorkspace workspace{img, 20};

    // Create search tile with some transparent pixels (index 0)
    PixelTile<IndexPixel> search_tile;
    for (std::size_t i = 0; i < 64; ++i) {
        // Half transparent, half non-transparent
        search_tile.set(i, IndexPixel{static_cast<std::size_t>(i % 2 == 0 ? 0 : 1)});
    }
    CanonicalPixelTile<IndexPixel> canonical_search_tile{search_tile};

    auto result = workspace.first_occurrence_of_by_color(canonical_search_tile, palette);
    EXPECT_FALSE(result.has_value());
}

#include "gtest/gtest.h"

#include <set>
#include <string>
#include <vector>

#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/anim_key_frame_mangler.hpp"
#include "porytiles2/infra/services/ascii_tile_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

namespace {

/**
 * @brief Creates a test palette with distinct colors at each index.
 */
Palette<Rgba32, pal::max_size> create_test_palette()
{
    Palette<Rgba32, pal::max_size> pal;
    // Index 0: transparent (black with alpha 0)
    pal.set(0, Rgba32{0, 0, 0, 0});
    // Index 1: red
    pal.set(1, Rgba32{255, 0, 0, 255});
    // Index 2: green
    pal.set(2, Rgba32{0, 255, 0, 255});
    // Index 3: blue
    pal.set(3, Rgba32{0, 0, 255, 255});
    // Index 4: yellow
    pal.set(4, Rgba32{255, 255, 0, 255});
    // Index 5: cyan
    pal.set(5, Rgba32{0, 255, 255, 255});
    // Index 6: magenta
    pal.set(6, Rgba32{255, 0, 255, 255});
    // Index 7: white
    pal.set(7, Rgba32{255, 255, 255, 255});
    // Indices 8-15: variations of grey
    for (std::size_t i = 8; i < 16; ++i) {
        const auto grey = static_cast<std::uint8_t>(i * 16);
        pal.set(i, Rgba32{grey, grey, grey, 255});
    }
    return pal;
}

/**
 * @brief Creates a tile with two colors: one in corners, one everywhere else.
 */
PixelTile<IndexPixel> create_two_color_tile(std::size_t corner_color, std::size_t other_color)
{
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{other_color});
    }
    // Set corners to corner_color
    tile.set(0, IndexPixel{corner_color});  // (0,0)
    tile.set(7, IndexPixel{corner_color});  // (0,7)
    tile.set(56, IndexPixel{corner_color}); // (7,0)
    tile.set(63, IndexPixel{corner_color}); // (7,7)
    return tile;
}

std::vector<const Palette<Rgba32, pal::max_size> *>
make_uniform_pal_ptrs(const Palette<Rgba32, pal::max_size> &pal, std::size_t count)
{
    return std::vector<const Palette<Rgba32, pal::max_size> *>(count, &pal);
}

} // namespace

class AnimKeyFrameManglerTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        formatter_ = std::make_unique<PlainTextFormatter>();
        tile_printer_ = std::make_unique<AsciiTilePrinter>(formatter_.get());
        palette_ = create_test_palette();
    }

    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<AsciiTilePrinter> tile_printer_;
    Palette<Rgba32, pal::max_size> palette_;
};

TEST_F(AnimKeyFrameManglerTests, shouldPassthroughWhenNoDuplicates)
{
    // arrange
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(create_two_color_tile(1, 2)); // Red corners, green fill
    tiles.push_back(create_two_color_tile(3, 4)); // Blue corners, yellow fill
    tiles.push_back(create_two_color_tile(5, 6)); // Cyan corners, magenta fill

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().tiles.size(), 3);
    EXPECT_TRUE(result.value().mangle_records.empty());
    // Tiles should be unchanged
    EXPECT_EQ(result.value().tiles[0], tiles[0]);
    EXPECT_EQ(result.value().tiles[1], tiles[1]);
    EXPECT_EQ(result.value().tiles[2], tiles[2]);
}

TEST_F(AnimKeyFrameManglerTests, shouldMangleSimpleDuplicatePair)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2); // Red corners, green fill
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // Duplicate!

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().tiles.size(), 2);
    EXPECT_EQ(result.value().mangle_records.size(), 1);

    // First tile should be unchanged
    EXPECT_EQ(result.value().tiles[0], original_tile);

    // Second tile should be different (mangled)
    EXPECT_NE(result.value().tiles[1], original_tile);

    // Both tiles should now be unique
    EXPECT_NE(result.value().tiles[0], result.value().tiles[1]);

    // Verify the mangle record
    const auto &record = *result.value().mangle_records.begin();
    EXPECT_EQ(record.tile_index, 1); // Second tile was mangled
    EXPECT_NE(record.original_pixel, record.mangled_pixel);
}

TEST_F(AnimKeyFrameManglerTests, shouldMangleMultipleDuplicatesOfSameTile)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2);
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // First duplicate
    tiles.push_back(original_tile); // Second duplicate

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().tiles.size(), 3);
    EXPECT_EQ(result.value().mangle_records.size(), 2); // Two tiles needed mangling

    // All three tiles should now be unique
    EXPECT_NE(result.value().tiles[0], result.value().tiles[1]);
    EXPECT_NE(result.value().tiles[0], result.value().tiles[2]);
    EXPECT_NE(result.value().tiles[1], result.value().tiles[2]);
}

TEST_F(AnimKeyFrameManglerTests, shouldAvoidDuplicatingExistingTiles)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2);
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // Duplicate

    // Create a tile that matches what a naive mangle might produce
    // The mangler should avoid creating this
    auto preexisting_tile = original_tile;
    preexisting_tile.set(0, IndexPixel{2}); // Change corner from red to green

    std::set<PixelTile<IndexPixel>> existing_tiles;
    existing_tiles.insert(preexisting_tile);

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());

    // The mangled tile should NOT match the preexisting tile
    EXPECT_NE(result.value().tiles[1], preexisting_tile);

    // Both result tiles should be unique against existing_tiles too
    EXPECT_EQ(existing_tiles.find(result.value().tiles[0]), existing_tiles.end());
    // Note: tiles[0] equals original_tile which is already NOT in existing_tiles (only preexisting_tile is)
}

TEST_F(AnimKeyFrameManglerTests, shouldNotMangleIntoCollisionWithExistingCanonicalTile)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2); // Red corners, green fill
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // Duplicate — forces a mangle

    // Create the tile that a naive mangle would produce:
    // The mangler tries pixel 0 first (corner at index 0) and swaps color 1 (red)
    // to its nearest palette neighbor. With our test palette, the nearest neighbor
    // to red (255,0,0) is index 8 (grey 128,128,128) at distance 48897.
    auto naive_mangle_tile = original_tile;
    naive_mangle_tile.set(0, IndexPixel{8}); // What the mangler's first attempt would produce

    // Insert the canonical form of this naive mangle into existing tiles,
    // simulating an unrelated tile in tiles.png that happens to match
    CanonicalPixelTile<IndexPixel> naive_canonical{naive_mangle_tile};
    const PixelTile<IndexPixel> &naive_base = naive_canonical;

    std::set<PixelTile<IndexPixel>> existing_canonical_tiles;
    existing_canonical_tiles.insert(naive_base);

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_canonical_tiles);

    // assert
    ASSERT_TRUE(result.has_value());

    // The mangled tile should NOT match the naive mangle (mangler had to skip it)
    EXPECT_NE(result.value().tiles[1], naive_mangle_tile);

    // The mangled tile should still be different from the original
    EXPECT_NE(result.value().tiles[1], original_tile);

    // The canonical form of the mangled tile should not collide with existing_canonical_tiles
    CanonicalPixelTile<IndexPixel> result_canonical{result.value().tiles[1]};
    const PixelTile<IndexPixel> &result_base = result_canonical;
    EXPECT_EQ(existing_canonical_tiles.find(result_base), existing_canonical_tiles.end());
}

TEST_F(AnimKeyFrameManglerTests, shouldPreservePaletteIndex)
{
    // arrange - create tile with true-color encoding (palette index in upper 4 bits)
    PixelTile<IndexPixel> tile;
    const std::size_t pal_index = 5; // Use palette 5
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        // Encode as (pal_index << 4) | color_index
        tile.set(i, IndexPixel{(pal_index << 4) | 2}); // All green in palette 5
    }
    // Set corners to a different color so we have 2 colors
    tile.set(0, IndexPixel{(pal_index << 4) | 1}); // Red in palette 5
    tile.set(7, IndexPixel{(pal_index << 4) | 1});
    tile.set(56, IndexPixel{(pal_index << 4) | 1});
    tile.set(63, IndexPixel{(pal_index << 4) | 1});

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);

    // The mangled pixel should preserve the palette index
    const auto &record = *result.value().mangle_records.begin();
    EXPECT_EQ(record.mangled_pixel.palette_index(), pal_index);
}

TEST_F(AnimKeyFrameManglerTests, shouldEmitRemarkWhenMangling)
{
    // arrange
    const auto tile = create_two_color_tile(1, 2);
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());

    // Check that a remark was emitted
    const auto &tag_counts = diag_->remark_tag_counts();
    auto it = tag_counts.find("anim-key-frame-mangle");
    ASSERT_NE(it, tag_counts.end());
    EXPECT_EQ(it->second, 1); // One mangle, one remark
}

TEST_F(AnimKeyFrameManglerTests, shouldMangleSolidColorTile)
{
    // arrange - solid color tile (all pixels same color)
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{1}); // All red
    }

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert - should SUCCEED by introducing a different palette color
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);

    // Tiles should now be unique
    EXPECT_NE(result.value().tiles[0], result.value().tiles[1]);

    // The mangled pixel should use a different color from the palette
    const auto &record = *result.value().mangle_records.begin();
    EXPECT_NE(record.original_pixel.color_index(), record.mangled_pixel.color_index());
}

TEST_F(AnimKeyFrameManglerTests, shouldHandleTransparentPixels)
{
    // arrange - create tile with transparent pixels (index 0) and two colors
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{0}); // Mostly transparent
    }
    // Add two non-transparent colors
    tile.set(9, IndexPixel{1});  // Red in interior
    tile.set(10, IndexPixel{2}); // Green in interior

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);

    // Mangled pixel should not be at a transparent position
    const auto &record = *result.value().mangle_records.begin();
    EXPECT_FALSE(record.original_pixel.is_transparent());
}

TEST_F(AnimKeyFrameManglerTests, shouldMakeMangledRecordsAccurate)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2);
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // Duplicate

    std::set<PixelTile<IndexPixel>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_tiles);

    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().mangle_records.size(), 1);

    const auto &record = *result.value().mangle_records.begin();

    // The record should accurately describe the change
    // Original pixel at the recorded index should match record.original_pixel
    EXPECT_EQ(original_tile.at(record.pixel_index), record.original_pixel);

    // Mangled tile at the recorded index should match record.mangled_pixel
    EXPECT_EQ(result.value().tiles[record.tile_index].at(record.pixel_index), record.mangled_pixel);
}

TEST_F(AnimKeyFrameManglerTests, shouldTreatFlipEquivalentTilesAsDuplicates)
{
    // arrange - create a tile and its horizontal flip (canonically equivalent)
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{2}); // Green fill
    }
    // Create asymmetric pattern: red on left side only
    tile.set(0, IndexPixel{1});  // Top-left corner
    tile.set(8, IndexPixel{1});  // Left side
    tile.set(56, IndexPixel{1}); // Bottom-left corner

    // Create the horizontal flip
    PixelTile<IndexPixel> h_flipped_tile = tile.flip(true, false);

    // Verify they ARE different as raw tiles
    ASSERT_NE(tile, h_flipped_tile);

    // But they SHOULD be canonically equivalent
    CanonicalPixelTile<IndexPixel> canonical_tile{tile};
    CanonicalPixelTile<IndexPixel> canonical_h_flipped{h_flipped_tile};
    const PixelTile<IndexPixel> &tile_base = canonical_tile;
    const PixelTile<IndexPixel> &h_flipped_base = canonical_h_flipped;
    ASSERT_EQ(tile_base, h_flipped_base) << "Test setup error: tiles should be canonically equivalent";

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(h_flipped_tile); // Flip-equivalent duplicate

    // Pass empty existing_canonical_tiles - the mangler will detect the duplicate
    // within the batch itself using canonical comparison
    std::set<PixelTile<IndexPixel>> existing_canonical_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim", tiles, make_uniform_pal_ptrs(palette_, tiles.size()), Rgba32{}, existing_canonical_tiles);

    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1) << "One tile should be mangled due to canonical equivalence";

    // After mangling, both result tiles should be canonically unique
    CanonicalPixelTile<IndexPixel> result_canonical_0{result.value().tiles[0]};
    CanonicalPixelTile<IndexPixel> result_canonical_1{result.value().tiles[1]};
    const PixelTile<IndexPixel> &result_base_0 = result_canonical_0;
    const PixelTile<IndexPixel> &result_base_1 = result_canonical_1;
    EXPECT_NE(result_base_0, result_base_1) << "Result tiles should be canonically unique";
}

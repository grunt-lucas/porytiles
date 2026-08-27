#include "gtest/gtest.h"

#include <set>
#include <string>
#include <vector>

#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/services/anim_key_frame_mangler.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles;

namespace {

Palette<Rgba32, palette::max_size> create_test_palette()
{
    Palette<Rgba32, palette::max_size> palette;
    // Index 0: transparent (black with alpha 0)
    palette.set(0, Rgba32{0, 0, 0, 0});
    // Index 1: red
    palette.set(1, Rgba32{255, 0, 0, 255});
    // Index 2: green
    palette.set(2, Rgba32{0, 255, 0, 255});
    // Index 3: blue
    palette.set(3, Rgba32{0, 0, 255, 255});
    // Index 4: yellow
    palette.set(4, Rgba32{255, 255, 0, 255});
    // Index 5: cyan
    palette.set(5, Rgba32{0, 255, 255, 255});
    // Index 6: magenta
    palette.set(6, Rgba32{255, 0, 255, 255});
    // Index 7: white
    palette.set(7, Rgba32{255, 255, 255, 255});
    // Indices 8-15: variations of grey
    for (std::size_t i = 8; i < 16; ++i) {
        const auto grey = static_cast<std::uint8_t>(i * 16);
        palette.set(i, Rgba32{grey, grey, grey, 255});
    }
    return palette;
}

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

std::vector<const Palette<Rgba32, palette::max_size> *>
make_uniform_palette_ptrs(const Palette<Rgba32, palette::max_size> &palette, std::size_t count)
{
    return std::vector<const Palette<Rgba32, palette::max_size> *>(count, &palette);
}

std::set<PixelTile<Rgba32>> make_canonical_rgba_set(
    const std::vector<PixelTile<IndexPixel>> &tiles, const Palette<Rgba32, palette::max_size> &palette)
{
    std::set<PixelTile<Rgba32>> result;
    for (const auto &tile : tiles) {
        result.insert(canonical_color_tile_from_index_tile(tile, palette, Rgba32{}));
    }
    return result;
}

std::vector<const std::set<PixelTile<Rgba32>> *>
make_existing_ptrs(const std::set<PixelTile<Rgba32>> &existing, std::size_t count)
{
    return std::vector<const std::set<PixelTile<Rgba32>> *>(count, &existing);
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
    Palette<Rgba32, palette::max_size> palette_;
};

TEST_F(AnimKeyFrameManglerTests, shouldPassthroughWhenNoDuplicates)
{
    // arrange
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(create_two_color_tile(1, 2)); // Red corners, green fill
    tiles.push_back(create_two_color_tile(3, 4)); // Blue corners, yellow fill
    tiles.push_back(create_two_color_tile(5, 6)); // Cyan corners, magenta fill

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

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

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

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
    ASSERT_FALSE(record.pixel_changes.empty());
    EXPECT_NE(record.pixel_changes[0].original_pixel, record.pixel_changes[0].mangled_pixel);
}

TEST_F(AnimKeyFrameManglerTests, shouldMangleMultipleDuplicatesOfSameTile)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2);
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // First duplicate
    tiles.push_back(original_tile); // Second duplicate

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

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

    const auto existing_tiles = make_canonical_rgba_set({preexisting_tile}, palette_);

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    // assert
    ASSERT_TRUE(result.has_value());

    // The mangled tile should NOT match the preexisting tile
    EXPECT_NE(result.value().tiles[1], preexisting_tile);

    // Both result tiles should be unique against existing_tiles too (compared in decoded RGBA space)
    EXPECT_FALSE(
        existing_tiles.contains(canonical_color_tile_from_index_tile(result.value().tiles[0], palette_, Rgba32{})));
    EXPECT_FALSE(
        existing_tiles.contains(canonical_color_tile_from_index_tile(result.value().tiles[1], palette_, Rgba32{})));
}

TEST_F(AnimKeyFrameManglerTests, shouldNotMangleIntoCollisionWithExistingCanonicalTile)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2); // Red corners, green fill
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // Duplicate, forces a mangle

    // Create the tile that a naive mangle would produce:
    // The mangler tries pixel 0 first (corner at index 0) and swaps color 1 (red)
    // to its nearest palette neighbor. With our test palette, the nearest neighbor
    // to red (255,0,0) is index 8 (grey 128,128,128) at distance 48897.
    auto naive_mangle_tile = original_tile;
    naive_mangle_tile.set(0, IndexPixel{8}); // What the mangler's first attempt would produce

    // Insert the canonical decoded form of this naive mangle into existing tiles,
    // simulating an unrelated tile in tiles.png that happens to match
    const auto existing_canonical_tiles = make_canonical_rgba_set({naive_mangle_tile}, palette_);

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_canonical_tiles, tiles.size()));

    // assert
    ASSERT_TRUE(result.has_value());

    // The mangled tile should NOT match the naive mangle (mangler had to skip it)
    EXPECT_NE(result.value().tiles[1], naive_mangle_tile);

    // The mangled tile should still be different from the original
    EXPECT_NE(result.value().tiles[1], original_tile);

    // The canonical decoded form of the mangled tile should not collide with existing_canonical_tiles
    EXPECT_FALSE(existing_canonical_tiles.contains(
        canonical_color_tile_from_index_tile(result.value().tiles[1], palette_, Rgba32{})));
}

TEST_F(AnimKeyFrameManglerTests, shouldPreservePaletteIndex)
{
    // arrange - create tile with true-color encoding (palette index in upper 4 bits)
    PixelTile<IndexPixel> tile;
    const std::size_t palette_index = 5; // Use palette 5
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        // Encode as (palette_index << 4) | color_index
        tile.set(i, IndexPixel{(palette_index << 4) | 2}); // All green in palette 5
    }
    // Set corners to a different color so we have 2 colors
    tile.set(0, IndexPixel{(palette_index << 4) | 1}); // Red in palette 5
    tile.set(7, IndexPixel{(palette_index << 4) | 1});
    tile.set(56, IndexPixel{(palette_index << 4) | 1});
    tile.set(63, IndexPixel{(palette_index << 4) | 1});

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    // assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);

    // The mangled pixel should preserve the palette index
    const auto &record = *result.value().mangle_records.begin();
    ASSERT_FALSE(record.pixel_changes.empty());
    EXPECT_EQ(record.pixel_changes[0].mangled_pixel.palette_index(), palette_index);
}

TEST_F(AnimKeyFrameManglerTests, shouldEmitRemarkWhenMangling)
{
    // arrange
    const auto tile = create_two_color_tile(1, 2);
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

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

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    // assert - should SUCCEED by introducing a different palette color
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);

    // Tiles should now be unique
    EXPECT_NE(result.value().tiles[0], result.value().tiles[1]);

    // The mangled pixel should use a different color from the palette
    const auto &record = *result.value().mangle_records.begin();
    ASSERT_FALSE(record.pixel_changes.empty());
    EXPECT_NE(
        record.pixel_changes[0].original_pixel.color_index(), record.pixel_changes[0].mangled_pixel.color_index());
}

TEST_F(AnimKeyFrameManglerTests, shouldHandleMostlyTransparentTiles)
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

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    // assert - should succeed; the mangler can mangle any pixel (including transparent ones)
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);
    EXPECT_NE(result.value().tiles[0], result.value().tiles[1]);
}

TEST_F(AnimKeyFrameManglerTests, shouldMangleFullyTransparentTiles)
{
    // arrange - all pixels are transparent (color index 0)
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{0});
    }

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    // assert - should succeed by swapping a transparent pixel to a non-transparent color
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);
    EXPECT_NE(result.value().tiles[0], result.value().tiles[1]);

    // The mangled pixel should now be non-transparent (swapped from index 0 to some other index)
    const auto &record = *result.value().mangle_records.begin();
    ASSERT_FALSE(record.pixel_changes.empty());
    EXPECT_TRUE(record.pixel_changes[0].original_pixel.is_transparent());
    EXPECT_FALSE(record.pixel_changes[0].mangled_pixel.is_transparent());
}

TEST_F(AnimKeyFrameManglerTests, shouldMakeMangledRecordsAccurate)
{
    // arrange
    const auto original_tile = create_two_color_tile(1, 2);
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(original_tile);
    tiles.push_back(original_tile); // Duplicate

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    // assert
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().mangle_records.size(), 1);

    const auto &record = *result.value().mangle_records.begin();
    ASSERT_FALSE(record.pixel_changes.empty());

    // The record should accurately describe the change
    for (const auto &change : record.pixel_changes) {
        // Original pixel at the recorded index should match change.original_pixel
        EXPECT_EQ(original_tile.at(change.pixel_index), change.original_pixel);

        // Mangled tile at the recorded index should match change.mangled_pixel
        EXPECT_EQ(result.value().tiles[record.tile_index].at(change.pixel_index), change.mangled_pixel);
    }
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
    std::set<PixelTile<Rgba32>> existing_canonical_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_canonical_tiles, tiles.size()));

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

TEST_F(AnimKeyFrameManglerTests, shouldMangleManySolidColorDuplicates)
{
    // arrange - 20 identical solid-color tiles
    // A solid tile has 16 unique canonical pixel positions under the 4-fold flip symmetry (64 / 4 = 16).
    // With only 1 alternative color per pixel, the old algorithm could produce at most 16 unique mangles
    // and would fail for the 18th+ duplicate. With all alternatives tried, the search space expands to
    // 16 positions * 14 alternative colors = 224, which is more than sufficient.
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{1}); // All red
    }

    constexpr std::size_t num_tiles = 20;
    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.reserve(num_tiles);
    for (std::size_t i = 0; i < num_tiles; ++i) {
        tiles.push_back(tile);
    }

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    // act
    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    // assert - should succeed with the expanded search space
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().tiles.size(), num_tiles);
    EXPECT_EQ(result.value().mangle_records.size(), num_tiles - 1); // All but first need mangling

    // All tiles should be canonically unique
    std::set<PixelTile<IndexPixel>> canonical_tiles;
    for (const auto &t : result.value().tiles) {
        CanonicalPixelTile<IndexPixel> canonical{t};
        const PixelTile<IndexPixel> &base = canonical;
        EXPECT_TRUE(canonical_tiles.insert(base).second) << "Found duplicate canonical tile after mangling";
    }
}

TEST_F(AnimKeyFrameManglerTests, MangleAvoidsDuplicateColorSlots)
{
    // Slot 9 duplicates slot 2's color. The closest alternative to green (distance 0) is slot 9, but swapping to it
    // would leave the decoded tile unchanged, so the mangler must pick a slot with a genuinely different color.
    auto palette = create_test_palette();
    palette.set(9, palette.at(2));

    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{2}); // Solid green
    }

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette, tiles.size()),
        Rgba32{},
        make_existing_ptrs(existing_tiles, tiles.size()));

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().mangle_records.size(), 1);

    // The decoded tiles must differ; index-only uniqueness (e.g. swapping slot 2 for its color-identical slot 9) is
    // not enough
    const auto decoded_0 = canonical_color_tile_from_index_tile(result.value().tiles[0], palette, Rgba32{});
    const auto decoded_1 = canonical_color_tile_from_index_tile(result.value().tiles[1], palette, Rgba32{});
    EXPECT_NE(decoded_0, decoded_1);
}

TEST_F(AnimKeyFrameManglerTests, MangleSkipsExtrinsicTransparencyColor)
{
    // Extrinsic transparency equals slot 9's color, the closest alternative to a solid slot-8 tile. Swapping to slot
    // 9 would decode to a transparent pixel that recompiles to index 0, so the mangler must skip it.
    const Rgba32 extrinsic = palette_.at(9);

    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{8}); // Solid grey
    }

    std::vector<PixelTile<IndexPixel>> tiles;
    tiles.push_back(tile);
    tiles.push_back(tile); // Duplicate

    std::set<PixelTile<Rgba32>> existing_tiles;

    AnimKeyFrameMangler mangler{diag_.get(), tile_printer_.get()};

    const auto result = mangler.mangle_duplicates(
        "test_anim",
        tiles,
        make_uniform_palette_ptrs(palette_, tiles.size()),
        extrinsic,
        make_existing_ptrs(existing_tiles, tiles.size()));

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().mangle_records.size(), 1);

    // No mangled pixel may decode to the extrinsic transparency color
    const auto &record = *result.value().mangle_records.begin();
    for (const auto &change : record.pixel_changes) {
        EXPECT_FALSE(palette_.at(change.mangled_pixel.color_index()).is_transparent(extrinsic));
    }
}

#include "gtest/gtest.h"

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "porytiles/domain/algorithms/tile_converters.hpp"
#include "porytiles/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_multi_palette_subtile_resolution_strategy.hpp"
#include "porytiles/domain/config/import_transparency_mode.hpp"
#include "porytiles/domain/config/per_anim_override.hpp"
#include "porytiles/domain/config/per_anim_overrides.hpp"
#include "porytiles/domain/models/anim_frame.hpp"
#include "porytiles/domain/models/anim_params.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/canonical_pixel_tile.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/services/anim_decompiler.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/infra/services/color_palette_printer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_domain_config.hpp"

using namespace porytiles;

namespace {

Palette<Rgba32, palette::max_size> create_test_palette()
{
    Palette<Rgba32, palette::max_size> palette;
    palette.set(0, Rgba32{0, 0, 0, 255});
    palette.set(1, Rgba32{255, 0, 0, 255});
    palette.set(2, Rgba32{0, 255, 0, 255});
    palette.set(3, Rgba32{0, 0, 255, 255});
    palette.set(4, Rgba32{255, 255, 0, 255});
    palette.set(5, Rgba32{0, 255, 255, 255});
    palette.set(6, Rgba32{255, 0, 255, 255});
    palette.set(7, Rgba32{255, 255, 255, 255});
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
    tile.set(0, IndexPixel{corner_color});
    tile.set(7, IndexPixel{corner_color});
    tile.set(56, IndexPixel{corner_color});
    tile.set(63, IndexPixel{corner_color});
    return tile;
}

PixelTile<IndexPixel> create_asymmetric_tile(std::size_t left_color, std::size_t fill_color)
{
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{fill_color});
    }
    // Left column only (col 0), rows 0-3
    tile.set(0, IndexPixel{left_color});  // row 0, col 0
    tile.set(8, IndexPixel{left_color});  // row 1, col 0
    tile.set(16, IndexPixel{left_color}); // row 2, col 0
    tile.set(24, IndexPixel{left_color}); // row 3, col 0
    return tile;
}

/// @brief Builds a tiles.png Image from a vector of tiles laid out in a single row of 8-pixel-wide columns.
///
/// @details
/// Tiles are laid out left-to-right, each 8x8 pixels. The resulting image width is num_tiles * 8.
Image<IndexPixel> build_tiles_png(const std::vector<PixelTile<IndexPixel>> &tiles)
{
    const std::size_t num_tiles = tiles.size();
    const std::size_t img_width = num_tiles * tile::side_length_pix;
    const std::size_t img_height = tile::side_length_pix;
    Image<IndexPixel> img{img_width, img_height};

    for (std::size_t t = 0; t < num_tiles; ++t) {
        for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
            for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                const std::size_t pixel_idx = row * tile::side_length_pix + col;
                const std::size_t img_col = t * tile::side_length_pix + col;
                img.set(row, img_col, tiles[t].at(pixel_idx));
            }
        }
    }

    return img;
}

Animation<IndexPixel> create_test_animation(
    const std::string &name,
    std::size_t tile_offset,
    std::size_t tile_count,
    const std::vector<PixelTile<IndexPixel>> &frame_tiles,
    const Palette<Rgba32, palette::max_size> &palette)
{
    AnimParams params;
    params.tile_offset(tile_offset);
    params.tile_count(tile_count);
    params.frame_names({DynamicCasedName{"0"}});
    params.frame_order({DynamicCasedName{"0"}});

    Animation<IndexPixel> anim{name, params};

    // Build palette as Palette<Rgba32> (dynamic size) for the frame
    Palette<Rgba32> frame_palette;
    for (std::size_t i = 0; i < palette::max_size; ++i) {
        frame_palette.add(palette.at(i));
    }

    AnimFrame<IndexPixel> frame{"0", frame_tiles};
    frame.palette(frame_palette);
    anim.put_frame("0", std::move(frame));

    return anim;
}

PorymapTilesetComponent build_porymap_component(
    const std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> &palettes,
    const std::vector<TilemapEntry> &metatiles,
    const Image<IndexPixel> &tiles_png)
{
    PorymapTilesetComponent component;
    for (std::size_t i = 0; i < palettes.size(); ++i) {
        component.set_palette(i, palettes[i]);
    }
    component.metatiles_bin(metatiles);
    component.tiles_png(tiles_png);
    return component;
}

/// @brief Builds an inter-animation duplicate detection set: the canonical decoded RGBA forms of @p tiles.
std::set<PixelTile<Rgba32>> make_inter_anim_rgba_set(
    const std::vector<PixelTile<IndexPixel>> &tiles,
    const Palette<Rgba32, palette::max_size> &palette,
    const Rgba32 &extrinsic_transparency)
{
    std::set<PixelTile<Rgba32>> result;
    for (const auto &tile : tiles) {
        result.insert(canonical_color_tile_from_index_tile(tile, palette, extrinsic_transparency));
    }
    return result;
}

} // namespace

class AnimDecompilerDuplicateDetectionTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        formatter_ = std::make_unique<PlainTextFormatter>();
        tile_printer_ = std::make_unique<AsciiTilePrinter>(formatter_.get());
        palette_printer_ = std::make_unique<ColorPalettePrinter>(formatter_.get());
        palette_ = create_test_palette();

        palettes_.fill(Palette<Rgba32, palette::max_size>{});
        palettes_[0] = palette_;
    }

    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<AsciiTilePrinter> tile_printer_;
    std::unique_ptr<ColorPalettePrinter> palette_printer_;
    Palette<Rgba32, palette::max_size> palette_;
    std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> palettes_;
    MockDomainConfig config_;
};

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldDetectCrossRangeExactDuplicate)
{
    // Tile 0 (non-animation) and tile 1 (animation) are identical
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto tiles_png = build_tiles_png({shared_tile, shared_tile});

    // Metatile references tile 1 with palette 0
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 1, {shared_tile}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    // With mangle strategy, it should succeed and produce a mangled tile
    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Decompilation should succeed with mangle strategy";

    // The key frame tile should have been mangled to differ from the external tile
    // We just verify decompilation succeeded. The mangler was invoked
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldDetectCrossRangeFlipEquivalentDuplicate)
{
    // Tile 0 (non-animation) is the h-flip of tile 1 (animation)
    const auto asymmetric_tile = create_asymmetric_tile(1, 2);
    const auto h_flipped_tile = asymmetric_tile.flip(true, false);

    // Verify they are raw-different but canonically equivalent
    ASSERT_NE(asymmetric_tile, h_flipped_tile);
    CanonicalPixelTile<IndexPixel> c1{asymmetric_tile};
    CanonicalPixelTile<IndexPixel> c2{h_flipped_tile};
    const PixelTile<IndexPixel> &b1 = c1;
    const PixelTile<IndexPixel> &b2 = c2;
    ASSERT_EQ(b1, b2) << "Test setup: tiles should be canonically equivalent";

    const auto tiles_png = build_tiles_png({asymmetric_tile, h_flipped_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};
    auto anim = create_test_animation("test_anim", 1, 1, {h_flipped_tile}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // With error strategy, it should fail (duplicate detected)
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler error_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = error_decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(error_result.has_value()) << "Should detect flip-equivalent cross-range duplicate";

    // With mangle strategy, it should succeed
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler mangle_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto mangle_result = mangle_decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_TRUE(mangle_result.has_value()) << "Mangle strategy should resolve flip-equivalent cross-range duplicate";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldDetectIntraAnimationFlipEquivalentDuplicate)
{
    // Two animation tiles that are flip-equivalent but raw-different
    const auto asymmetric_tile = create_asymmetric_tile(1, 2);
    const auto h_flipped_tile = asymmetric_tile.flip(true, false);
    const auto unique_tile = create_two_color_tile(3, 4);

    ASSERT_NE(asymmetric_tile, h_flipped_tile);

    // tiles.png: tile 0 is unique (non-anim), tiles 1-2 are the animation range
    const auto tiles_png = build_tiles_png({unique_tile, asymmetric_tile, h_flipped_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 2, {asymmetric_tile, h_flipped_tile}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Error strategy should detect intra-animation flip-equivalent duplicate
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler error_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = error_decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(error_result.has_value()) << "Should detect intra-animation flip-equivalent duplicate";

    // Mangle strategy should resolve it
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler mangle_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto mangle_result = mangle_decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_TRUE(mangle_result.has_value())
        << "Mangle strategy should resolve intra-animation flip-equivalent duplicate";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldNotFalsePositiveWhenNoDuplicates)
{
    // All tiles are distinct (not even flip-equivalent)
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 2, {tile_b, tile_c}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Even with error strategy, should succeed (no duplicates)
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_TRUE(result.has_value()) << "Should not false-positive when no duplicates exist";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldHandleMixedCrossRangeAndIntraAnimationDuplicates)
{
    // tile_a: non-animation tile
    // tile_b: animation tile 0, exact duplicate of tile_a (cross-range)
    // tile_c: animation tile 1, h-flip of tile_b (intra-animation flip-equivalent)
    const auto tile_a = create_asymmetric_tile(1, 2);
    const auto tile_b = tile_a;                   // exact cross-range duplicate
    const auto tile_c = tile_a.flip(true, false); // intra-animation flip-equivalent to tile_b

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 2, {tile_b, tile_c}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Error strategy should fail
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler error_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = error_decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(error_result.has_value()) << "Should detect mixed cross-range and intra-animation duplicates";

    // Mangle strategy should succeed
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler mangle_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto mangle_result = mangle_decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_TRUE(mangle_result.has_value()) << "Mangle strategy should resolve mixed duplicates";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldDetectInterAnimationExactDuplicate)
{
    // Two animations share an identical key frame tile
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto unique_non_anim = create_two_color_tile(5, 6);

    // tiles.png: tile 0 is non-anim, tile 1 is water's key frame, tile 2 is ocean's key frame (same as water's)
    const auto tiles_png = build_tiles_png({unique_non_anim, shared_tile, shared_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 0, false, false}};
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Simulate water's canonical tiles already processed
    const auto inter_anim_tiles = make_inter_anim_rgba_set({shared_tile}, palette_, config_.extrinsic_transparency);

    // Ocean animation at tile offset 2
    auto ocean_anim = create_test_animation("ocean", 2, 1, {shared_tile}, palette_);

    // Error strategy should detect the inter-animation duplicate
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler error_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = error_decompiler.decompile_animation("test_tileset", ocean_anim, inter_anim_tiles, component);

    EXPECT_FALSE(error_result.has_value()) << "Should detect inter-animation exact duplicate";

    // Mangle strategy should resolve it
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler mangle_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto mangle_result = mangle_decompiler.decompile_animation("test_tileset", ocean_anim, inter_anim_tiles, component);

    EXPECT_TRUE(mangle_result.has_value()) << "Mangle strategy should resolve inter-animation exact duplicate";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldDetectInterAnimationFlipEquivalentDuplicate)
{
    // Water's tile and ocean's tile are flip-equivalent but raw-different
    const auto water_tile = create_asymmetric_tile(1, 2);
    const auto ocean_tile = water_tile.flip(true, false);
    const auto unique_non_anim = create_two_color_tile(5, 6);

    ASSERT_NE(water_tile, ocean_tile);
    CanonicalPixelTile<IndexPixel> c1{water_tile};
    CanonicalPixelTile<IndexPixel> c2{ocean_tile};
    const PixelTile<IndexPixel> &b1 = c1;
    const PixelTile<IndexPixel> &b2 = c2;
    ASSERT_EQ(b1, b2) << "Test setup: tiles should be canonically equivalent";

    // tiles.png: tile 0 is non-anim, tile 1 is water's, tile 2 is ocean's
    const auto tiles_png = build_tiles_png({unique_non_anim, water_tile, ocean_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 0, false, false}};
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Simulate water's canonical tiles already processed
    const auto inter_anim_tiles = make_inter_anim_rgba_set({water_tile}, palette_, config_.extrinsic_transparency);

    auto ocean_anim = create_test_animation("ocean", 2, 1, {ocean_tile}, palette_);

    // Error strategy should detect the inter-animation flip-equivalent duplicate
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler error_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = error_decompiler.decompile_animation("test_tileset", ocean_anim, inter_anim_tiles, component);

    EXPECT_FALSE(error_result.has_value()) << "Should detect inter-animation flip-equivalent duplicate";

    // Mangle strategy should resolve it
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler mangle_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto mangle_result = mangle_decompiler.decompile_animation("test_tileset", ocean_anim, inter_anim_tiles, component);

    EXPECT_TRUE(mangle_result.has_value())
        << "Mangle strategy should resolve inter-animation flip-equivalent duplicate";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldDetectInterAnimDuplicateWhenAnimNotInTilesPng)
{
    // Critical real-world case: models the land_waters_edge scenario. Water's key frame tiles are NOT present in
    // tiles.png at all (simulating an animation whose tiles were removed/never appeared in the tileset image). Ocean's
    // tiles ARE in tiles.png and are identical to water's. Without the inter-animation fix, this duplicate would be
    // completely missed because existing_canonical_tiles wouldn't contain water's tiles.
    const auto water_tile = create_two_color_tile(1, 2);
    const auto ocean_tile = water_tile; // identical to water's
    const auto unrelated_tile = create_two_color_tile(3, 4);

    // tiles.png: only contains unrelated_tile and ocean's tile, water's tile is NOT here
    const auto tiles_png = build_tiles_png({unrelated_tile, ocean_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Simulate water's canonical tiles from a previous decompilation
    const auto inter_anim_tiles = make_inter_anim_rgba_set({water_tile}, palette_, config_.extrinsic_transparency);

    // Ocean animation at tile offset 1
    auto ocean_anim = create_test_animation("ocean", 1, 1, {ocean_tile}, palette_);

    // Error strategy should correctly detect the inter-animation duplicate
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler error_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = error_decompiler.decompile_animation("test_tileset", ocean_anim, inter_anim_tiles, component);

    EXPECT_FALSE(error_result.has_value())
        << "Should detect inter-animation duplicate even when first animation is not in tiles.png";

    // Mangle strategy should also resolve it
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler mangle_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto mangle_result = mangle_decompiler.decompile_animation("test_tileset", ocean_anim, inter_anim_tiles, component);

    EXPECT_TRUE(mangle_result.has_value())
        << "Mangle strategy should resolve inter-animation duplicate when first animation is not in tiles.png";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldNotMiscategorizeInterAnimAsCrossRange)
{
    // Inter-animation duplicate should say "another animation's key frame tile", not "non-animation tile"
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto unique_non_anim = create_two_color_tile(5, 6); // completely different from shared_tile

    // tiles.png: tile 0 is non-anim (unique), tile 1 is ocean's key frame
    const auto tiles_png = build_tiles_png({unique_non_anim, shared_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Water's canonical tiles already processed (same as shared_tile)
    const auto inter_anim_tiles = make_inter_anim_rgba_set({shared_tile}, palette_, config_.extrinsic_transparency);

    auto ocean_anim = create_test_animation("ocean", 1, 1, {shared_tile}, palette_);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = decompiler.decompile_animation("test_tileset", ocean_anim, inter_anim_tiles, component);

    ASSERT_FALSE(error_result.has_value()) << "Should detect inter-animation duplicate";

    // Verify the error message mentions "another animation's key frame tile"
    const std::string error_text = error_result.error().join(*formatter_);
    EXPECT_TRUE(error_text.find("another animation's key frame tile") != std::string::npos)
        << "Error should mention 'another animation's key frame tile', got: " << error_text;
    EXPECT_TRUE(error_text.find("non-animation tile") == std::string::npos)
        << "Error should NOT mention 'non-animation tile', got: " << error_text;
}

TEST_F(AnimDecompilerDuplicateDetectionTests, DetectsColorIdenticalKeyFramesAcrossDuplicateSlots)
{
    // Slot 9 duplicates slot 2's color, so a solid slot-2 tile and a solid slot-9 tile differ in index space but
    // decode to identical colors. Compile-side key frame validation compares decoded colors, so the importer must
    // treat these as duplicates too.
    palette_.set(9, palette_.at(2));
    palettes_[0] = palette_;

    PixelTile<IndexPixel> solid_slot_2;
    PixelTile<IndexPixel> solid_slot_9;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        solid_slot_2.set(i, IndexPixel{2});
        solid_slot_9.set(i, IndexPixel{9});
    }
    ASSERT_NE(solid_slot_2, solid_slot_9);

    const auto unique_non_anim = create_two_color_tile(5, 6);
    const auto tiles_png = build_tiles_png({unique_non_anim, solid_slot_2, solid_slot_9});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 0, false, false}};
    auto anim = create_test_animation("test_anim", 1, 2, {solid_slot_2, solid_slot_9}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler error_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto error_result = error_decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(error_result.has_value()) << "Should detect color-identical key frame tiles across duplicate slots";

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler mangle_decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto mangle_result = mangle_decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(mangle_result.has_value()) << "Mangle strategy should resolve color-identical duplicate";

    // The decompiled key frame tiles must be distinct in canonical decoded RGBA space, since that is how compile-side
    // key frame validation compares them
    const auto &key_frame_tiles = mangle_result.value().key_frame().tiles();
    ASSERT_EQ(key_frame_tiles.size(), 2);
    const CanonicalPixelTile<Rgba32> canonical_0{key_frame_tiles[0]};
    const CanonicalPixelTile<Rgba32> canonical_1{key_frame_tiles[1]};
    const PixelTile<Rgba32> &base_0 = canonical_0;
    const PixelTile<Rgba32> &base_1 = canonical_1;
    EXPECT_NE(base_0, base_1);
}

namespace {

Palette<Rgba32, palette::max_size> create_second_test_palette()
{
    Palette<Rgba32, palette::max_size> palette;
    palette.set(0, Rgba32{0, 0, 0, 0});
    palette.set(1, Rgba32{128, 0, 0, 255});
    palette.set(2, Rgba32{0, 128, 0, 255});
    palette.set(3, Rgba32{0, 0, 128, 255});
    palette.set(4, Rgba32{128, 128, 0, 255});
    palette.set(5, Rgba32{0, 128, 128, 255});
    palette.set(6, Rgba32{128, 0, 128, 255});
    palette.set(7, Rgba32{128, 128, 128, 255});
    for (std::size_t i = 8; i < 16; ++i) {
        const auto val = static_cast<std::uint8_t>(i * 8 + 64);
        palette.set(i, Rgba32{val, val, val, 255});
    }
    return palette;
}

Animation<IndexPixel> create_test_animation_no_palette(
    const std::string &name,
    std::size_t tile_offset,
    std::size_t tile_count,
    const std::vector<PixelTile<IndexPixel>> &frame_tiles)
{
    AnimParams params;
    params.tile_offset(tile_offset);
    params.tile_count(tile_count);
    params.frame_names({DynamicCasedName{"0"}});
    params.frame_order({DynamicCasedName{"0"}});

    Animation<IndexPixel> anim{name, params};
    AnimFrame<IndexPixel> frame{"0", frame_tiles};
    anim.put_frame("0", std::move(frame));
    return anim;
}

} // namespace

class AnimDecompilerMultiPaletteTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        formatter_ = std::make_unique<PlainTextFormatter>();
        tile_printer_ = std::make_unique<AsciiTilePrinter>(formatter_.get());
        palette_printer_ = std::make_unique<ColorPalettePrinter>(formatter_.get());
        palette_0_ = create_test_palette();
        palette_1_ = create_second_test_palette();

        palettes_.fill(Palette<Rgba32, palette::max_size>{});
        palettes_[0] = palette_0_;
        palettes_[1] = palette_1_;
    }

    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<AsciiTilePrinter> tile_printer_;
    std::unique_ptr<ColorPalettePrinter> palette_printer_;
    Palette<Rgba32, palette::max_size> palette_0_;
    Palette<Rgba32, palette::max_size> palette_1_;
    std::array<Palette<Rgba32, palette::max_size>, palette::num_palettes> palettes_;
    MockDomainConfig config_;
};

TEST_F(AnimDecompilerMultiPaletteTests, shouldDecompileMultiPaletteAnimationWithScanLocalMetatiles)
{
    // Two subtiles: tile 1 uses palette 0, tile 2 uses palette 1
    const auto tile_a = create_two_color_tile(1, 2); // Non-animation tile
    const auto tile_b = create_two_color_tile(3, 4); // Anim subtile 0
    const auto tile_c = create_two_color_tile(5, 6); // Anim subtile 1

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // Metatile entries: tile 1 uses palette 0, tile 2 uses palette 1
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Multi-palette decompilation should succeed";

    // Verify key frame has 2 tiles
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);

    // Verify subtile 0 was decompiled with palette 0 colors
    const auto &key_tile_0 = result.value().key_frame().tile_at(0);
    // Corner pixel (index 0) should be color index 3 from palette 0 = Rgba32{0, 0, 255, 255}
    EXPECT_EQ(key_tile_0.at(0), palette_0_.at(3));

    // Verify subtile 1 was decompiled with palette 1 colors
    const auto &key_tile_1 = result.value().key_frame().tile_at(1);
    // Corner pixel (index 0) should be color index 5 from palette 1 = Rgba32{0, 128, 128, 255}
    EXPECT_EQ(key_tile_1.at(0), palette_1_.at(5));
}

// Index 0 corner pixels of a frame tile are transparent. The import transparency mode selects whether they are written
// as the extrinsic color or as alpha 0.
TEST_F(AnimDecompilerMultiPaletteTests, ImportTransparencyModeSelectsFrameTransparentPixel)
{
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(0, 3);
    const auto tiles_png = build_tiles_png({tile_a, tile_b});
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};
    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    for (const auto mode :
         {ImportTransparencyMode::extrinsic, ImportTransparencyMode::mixed, ImportTransparencyMode::alpha}) {
        config_.import_transparency = mode;
        AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

        auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);
        ASSERT_TRUE(result.has_value()) << "mode " << mode;

        // Rgba32 equality compares all four channels, so these also check alpha
        const Rgba32 expected = mode == ImportTransparencyMode::alpha ? Rgba32{} : config_.extrinsic_transparency;
        EXPECT_EQ(result.value().key_frame().tile_at(0).at(0), expected) << "mode " << mode;
        EXPECT_EQ(result.value().frames().at("0").tile_at(0).at(0), expected) << "mode " << mode;
        EXPECT_EQ(result.value().key_frame().tile_at(0).at(1), palette_0_.at(3));
    }
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldErrorWhenSubtileNotReferencedInMetatiles)
{
    // Two subtiles but only tile 1 is referenced in metatiles. Tile 2 is unresolved
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // Only tile 1 referenced. Tile 2 will be unresolved
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(result.has_value()) << "Should error when a subtile is not referenced in local metatiles";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldErrorWhenNoSubtilesReferencedInMetatiles)
{
    // No subtiles in the animation range are referenced in metatiles
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);

    const auto tiles_png = build_tiles_png({tile_a, tile_b});

    // No metatile references tile 1 at all
    std::vector<TilemapEntry> metatiles{TilemapEntry{0, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(result.has_value()) << "Should error when no subtiles are referenced in metatiles";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldErrorWhenSubtileHasConflictingPaletteIndices)
{
    // Single-subtile animation, but tile 1 is referenced with two different palette indices
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);

    const auto tiles_png = build_tiles_png({tile_a, tile_b});

    // Tile 1 referenced with palette 0 in one metatile and palette 1 in another
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{1, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(result.has_value()) << "Should error when a subtile has conflicting palette indices";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldMangleMultiPaletteDuplicateKeyFrameTiles)
{
    // Two subtiles using different palettes, but their key frame tiles are exact duplicates
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto unique_tile = create_two_color_tile(5, 6);

    // tiles.png: unique non-anim tile, then two identical anim tiles
    const auto tiles_png = build_tiles_png({unique_tile, shared_tile, shared_tile});

    // Tile 1 uses palette 0, tile 2 uses palette 1
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {shared_tile, shared_tile});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Mangle strategy should resolve multi-palette duplicate key frame tiles";
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldApplyPerSubtileExplicitPaletteStrategies)
{
    // Two subtiles: tile 1 and tile 2. Neither is referenced in metatiles.
    // Use AnimConfig with explicit palette strategies per subtile.
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // No metatile references tiles 1 or 2
    std::vector<TilemapEntry> metatiles{TilemapEntry{0, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Set up per-subtile strategies: subtile 0 -> palette_00, subtile 1 -> palette_01
    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.per_tile_palette_resolution_strategies = {
        AnimPaletteResolutionStrategy::palette_00, AnimPaletteResolutionStrategy::palette_01};
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Per-subtile explicit strategies should succeed";
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);

    // Verify subtile 0 was decompiled with palette 0 colors
    const auto &key_tile_0 = result.value().key_frame().tile_at(0);
    EXPECT_EQ(key_tile_0.at(0), palette_0_.at(3));

    // Verify subtile 1 was decompiled with palette 1 colors
    const auto &key_tile_1 = result.value().key_frame().tile_at(1);
    EXPECT_EQ(key_tile_1.at(0), palette_1_.at(5));
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldFallBackToGlobalWhenAnimNotInConfigs)
{
    // Animation is not in the configs map at all, should use global strategy
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);

    const auto tiles_png = build_tiles_png({tile_a, tile_b});

    // Tile 1 referenced with palette 0
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // anim_configs is empty, should use global (scan_local_metatiles)
    config_.per_anim_overrides = PerAnimOverrides{};

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Should fall back to global when anim not in configs";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldFallBackToGlobalForNulloptEntries)
{
    // AnimConfig exists with per_tile_palette_resolution_strategies containing a nullopt entry (underscore).
    // The nullopt subtile should fall back to the effective default (global, since no per-anim strategy is set).
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // Tile 2 referenced with palette 1 in metatiles (for the global scan to work)
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // subtile 0 -> explicit palette_00, subtile 1 -> nullopt (falls back to global scan_local_metatiles)
    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.per_tile_palette_resolution_strategies = {AnimPaletteResolutionStrategy::palette_00, std::nullopt};
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Nullopt entries should fall back to global strategy";
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);

    // Verify subtile 0 was decompiled with palette 0 colors (explicit)
    const auto &key_tile_0 = result.value().key_frame().tile_at(0);
    EXPECT_EQ(key_tile_0.at(0), palette_0_.at(3));

    // Verify subtile 1 was decompiled with palette 1 colors (from metatile scan)
    const auto &key_tile_1 = result.value().key_frame().tile_at(1);
    EXPECT_EQ(key_tile_1.at(0), palette_1_.at(5));
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldErrorWhenPaletteResolutionStrategiesSizeMismatch)
{
    // AnimConfig has 1 strategy entry but animation has 2 subtiles, should error
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // Wrong number of strategies, should cause a hard error
    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.per_tile_palette_resolution_strategies = {AnimPaletteResolutionStrategy::palette_00};
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(result.has_value()) << "Should error when per_tile_palette_resolution_strategies size != tile_count";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldHandleMixedScanAndExplicitPerSubtile)
{
    // Mixed strategies: subtile 0 uses scan_local_metatiles, subtile 1 uses explicit palette_01
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // Tile 1 referenced with palette 0 (for scan to find), tile 2 not referenced
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    // subtile 0 -> scan_local_metatiles, subtile 1 -> explicit palette_01
    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.per_tile_palette_resolution_strategies = {
        AnimPaletteResolutionStrategy::scan_local_metatiles, AnimPaletteResolutionStrategy::palette_01};
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Mixed scan + explicit strategies should succeed";
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);

    // Verify subtile 0 was decompiled with palette 0 colors (from scan)
    const auto &key_tile_0 = result.value().key_frame().tile_at(0);
    EXPECT_EQ(key_tile_0.at(0), palette_0_.at(3));

    // Verify subtile 1 was decompiled with palette 1 colors (explicit)
    const auto &key_tile_1 = result.value().key_frame().tile_at(1);
    EXPECT_EQ(key_tile_1.at(0), palette_1_.at(5));
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldApplyPerAnimStrategy)
{
    // AnimConfig has palette_resolution_strategy = palette_01 and no per-tile list.
    // All subtiles should use palette 1.
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // No metatile references tiles 1 or 2
    std::vector<TilemapEntry> metatiles{TilemapEntry{0, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.palette_resolution_strategy = AnimPaletteResolutionStrategy::palette_01;
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Per-anim strategy should apply to all subtiles";
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);

    // Both subtiles should use palette 1
    const auto &key_tile_0 = result.value().key_frame().tile_at(0);
    EXPECT_EQ(key_tile_0.at(0), palette_1_.at(3));

    const auto &key_tile_1 = result.value().key_frame().tile_at(1);
    EXPECT_EQ(key_tile_1.at(0), palette_1_.at(5));
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldOverridePerAnimWithPerTile)
{
    // AnimConfig has palette_resolution_strategy = palette_01 and per-tile [palette_00, _].
    // Subtile 0 should use palette 0 (per-tile override), subtile 1 should use palette 1 (per-anim fallback).
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // No metatile references tiles 1 or 2
    std::vector<TilemapEntry> metatiles{TilemapEntry{0, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.palette_resolution_strategy = AnimPaletteResolutionStrategy::palette_01;
    anim_cfg.per_tile_palette_resolution_strategies = {AnimPaletteResolutionStrategy::palette_00, std::nullopt};
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Per-tile should override per-anim for explicit entries";
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);

    // Subtile 0: per-tile palette_00
    const auto &key_tile_0 = result.value().key_frame().tile_at(0);
    EXPECT_EQ(key_tile_0.at(0), palette_0_.at(3));

    // Subtile 1: per-anim palette_01 (nullopt in per-tile falls to per-anim)
    const auto &key_tile_1 = result.value().key_frame().tile_at(1);
    EXPECT_EQ(key_tile_1.at(0), palette_1_.at(5));
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldFallBackFromPerTileNulloptToPerAnimThenGlobal)
{
    // AnimConfig has palette_resolution_strategy = palette_01 and per-tile [_, _].
    // Both subtiles have nullopt in per-tile, so they should use per-anim (palette_01), NOT global.
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    // Tile 1 referenced with palette 0 in metatiles (global scan would find palette 0, but per-anim should win)
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{2, 0, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 2, {tile_b, tile_c});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.palette_resolution_strategy = AnimPaletteResolutionStrategy::palette_01;
    anim_cfg.per_tile_palette_resolution_strategies = {std::nullopt, std::nullopt};
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Nullopt per-tile entries should fall to per-anim, not global";
    ASSERT_EQ(result.value().key_frame().tiles().size(), 2);

    // Both subtiles should use palette 1 (per-anim), not palette 0 (which global scan would find)
    const auto &key_tile_0 = result.value().key_frame().tile_at(0);
    EXPECT_EQ(key_tile_0.at(0), palette_1_.at(3));

    const auto &key_tile_1 = result.value().key_frame().tile_at(1);
    EXPECT_EQ(key_tile_1.at(0), palette_1_.at(5));
}

// ── Per-animation key_frame_resolution_strategy cascade tests ──────────────────

class AnimDecompilerKeyFrameStrategyTests : public AnimDecompilerDuplicateDetectionTests {};

TEST_F(AnimDecompilerKeyFrameStrategyTests, perAnimMangleOverridesGlobalError)
{
    // Global is error, but per-anim override is mangle. Decompilation should succeed
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto tiles_png = build_tiles_png({shared_tile, shared_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 1, {shared_tile}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_TRUE(result.has_value()) << "Per-anim mangle should override global error";
}

TEST_F(AnimDecompilerKeyFrameStrategyTests, perAnimErrorOverridesGlobalMangle)
{
    // Global is mangle, but per-anim override is error. Decompilation should fail
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto tiles_png = build_tiles_png({shared_tile, shared_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 1, {shared_tile}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::error;
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(result.has_value()) << "Per-anim error should override global mangle";
}

TEST_F(AnimDecompilerKeyFrameStrategyTests, nulloptPerAnimFallsBackToGlobal)
{
    // Per-anim key_frame_resolution_strategy is nullopt, should fall back to global mangle
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto tiles_png = build_tiles_png({shared_tile, shared_tile});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 1, {shared_tile}, palette_);
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    // key_frame_resolution_strategy left as std::nullopt (default)
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_TRUE(result.has_value()) << "Nullopt per-anim should fall back to global mangle";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldWarnAndPickLowestPaletteWhenMultiPaletteStrategyIsWarning)
{
    // Single-subtile animation, tile 1 referenced with palette 0 and palette 1
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);

    const auto tiles_png = build_tiles_png({tile_a, tile_b});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{1, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_multi_palette_subtile_resolution_strategy = AnimMultiPaletteSubtileResolutionStrategy::warning;
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;
    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Warning strategy should allow decompilation to succeed";

    const auto &key_tile = result.value().key_frame().tile_at(0);
    EXPECT_EQ(key_tile.at(0), palette_0_.at(3));

    EXPECT_FALSE(diag_->warnings().empty()) << "Should have emitted a warning about multi-palette subtile";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldErrorWhenMultiPaletteStrategyIsError)
{
    // Same scenario but with error strategy (default behavior)
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);

    const auto tiles_png = build_tiles_png({tile_a, tile_b});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{1, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_multi_palette_subtile_resolution_strategy = AnimMultiPaletteSubtileResolutionStrategy::error;
    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(result.has_value()) << "Error strategy should fail decompilation";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldApplyPerAnimMultiPaletteStrategyOverride)
{
    // Global = error, per-anim = warning. The per-anim override should win.
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);

    const auto tiles_png = build_tiles_png({tile_a, tile_b});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{1, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_multi_palette_subtile_resolution_strategy = AnimMultiPaletteSubtileResolutionStrategy::error;
    config_.global_anim_key_frame_resolution_strategy = AnimKeyFrameResolutionStrategy::mangle;

    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.multi_palette_subtile_resolution_strategy = AnimMultiPaletteSubtileResolutionStrategy::warning;
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    ASSERT_TRUE(result.has_value()) << "Per-anim warning should override global error";
    EXPECT_FALSE(diag_->warnings().empty()) << "Should have emitted a warning about multi-palette subtile";
}

TEST_F(AnimDecompilerMultiPaletteTests, shouldApplyPerAnimMultiPaletteErrorOverridingGlobalWarning)
{
    // Global = warning, per-anim = error. The per-anim override should win and cause a failure.
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);

    const auto tiles_png = build_tiles_png({tile_a, tile_b});

    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}, TilemapEntry{1, 1, false, false}};

    auto anim = create_test_animation_no_palette("test_anim", 1, 1, {tile_b});
    auto component = build_porymap_component(palettes_, metatiles, tiles_png);

    config_.global_anim_multi_palette_subtile_resolution_strategy = AnimMultiPaletteSubtileResolutionStrategy::warning;

    PerAnimOverride anim_cfg;
    anim_cfg.anim_name = "test_anim";
    anim_cfg.multi_palette_subtile_resolution_strategy = AnimMultiPaletteSubtileResolutionStrategy::error;
    config_.per_anim_overrides["test_anim"] = std::move(anim_cfg);

    AnimDecompiler decompiler{&config_, diag_.get(), tile_printer_.get(), palette_printer_.get()};

    auto result = decompiler.decompile_animation("test_tileset", anim, {}, component);

    EXPECT_FALSE(result.has_value()) << "Per-anim error should override global warning";
}

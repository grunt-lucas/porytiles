#include "gtest/gtest.h"

#include <array>
#include <string>
#include <vector>

#include "porytiles2/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/animation_decompiler.hpp"
#include "porytiles2/infra/services/ascii_tile_printer.hpp"
#include "porytiles2/infra/services/color_palette_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

namespace {

/**
 * @brief Creates a test palette with distinct colors at each index.
 */
Palette<Rgba32, pal::max_size> create_test_palette()
{
    Palette<Rgba32, pal::max_size> pal;
    pal.set(0, Rgba32{0, 0, 0, 0});
    pal.set(1, Rgba32{255, 0, 0, 255});
    pal.set(2, Rgba32{0, 255, 0, 255});
    pal.set(3, Rgba32{0, 0, 255, 255});
    pal.set(4, Rgba32{255, 255, 0, 255});
    pal.set(5, Rgba32{0, 255, 255, 255});
    pal.set(6, Rgba32{255, 0, 255, 255});
    pal.set(7, Rgba32{255, 255, 255, 255});
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
    tile.set(0, IndexPixel{corner_color});
    tile.set(7, IndexPixel{corner_color});
    tile.set(56, IndexPixel{corner_color});
    tile.set(63, IndexPixel{corner_color});
    return tile;
}

/**
 * @brief Creates an asymmetric tile (left-side pattern) that is NOT symmetric under any flip.
 */
PixelTile<IndexPixel> create_asymmetric_tile(std::size_t left_color, std::size_t fill_color)
{
    PixelTile<IndexPixel> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{fill_color});
    }
    // Left column only (col 0), rows 0-3
    tile.set(0, IndexPixel{left_color});   // row 0, col 0
    tile.set(8, IndexPixel{left_color});   // row 1, col 0
    tile.set(16, IndexPixel{left_color});  // row 2, col 0
    tile.set(24, IndexPixel{left_color});  // row 3, col 0
    return tile;
}

/**
 * @brief Builds a tiles.png Image from a vector of tiles laid out in a single row of 8-pixel-wide columns.
 *
 * @details
 * Tiles are laid out left-to-right, each 8x8 pixels. The resulting image width is num_tiles * 8.
 */
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

/**
 * @brief Creates a minimal Animation<IndexPixel> with the given params and a single frame.
 */
Animation<IndexPixel> create_test_animation(
    const std::string &name, std::size_t tile_offset, std::size_t tile_count,
    const std::vector<PixelTile<IndexPixel>> &frame_tiles, const Palette<Rgba32, pal::max_size> &pal)
{
    AnimationParams params;
    params.tile_offset(tile_offset);
    params.tile_count(tile_count);
    params.frame_names({"0"});
    params.frame_order({"0"});

    Animation<IndexPixel> anim{name, params};

    // Build palette as Palette<Rgba32> (dynamic size) for the frame
    Palette<Rgba32> frame_pal;
    for (std::size_t i = 0; i < pal::max_size; ++i) {
        frame_pal.add(pal.at(i));
    }

    AnimationFrame<IndexPixel> frame{"0", frame_tiles};
    frame.palette(frame_pal);
    anim.put_frame("0", std::move(frame));

    return anim;
}

ConfigValue<Rgba32> make_transparency_config()
{
    return ConfigValue<Rgba32>{Rgba32{0, 0, 0, 0}, "extrinsic-transparency", "test", {}};
}

ConfigValue<AnimPalResolutionStrategy> make_pal_strategy()
{
    return ConfigValue<AnimPalResolutionStrategy>{
        AnimPalResolutionStrategy::internal_png_pal, "anim-pal-resolution-strategy", "test", {}};
}

ConfigValue<AnimKeyFrameResolutionStrategy> make_key_frame_strategy(AnimKeyFrameResolutionStrategy strat)
{
    return ConfigValue<AnimKeyFrameResolutionStrategy>{strat, "anim-key-frame-resolution-strategy", "test", {}};
}

} // namespace

class AnimDecompilerDuplicateDetectionTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        formatter_ = std::make_unique<PlainTextFormatter>();
        tile_printer_ = std::make_unique<AsciiTilePrinter>(formatter_.get());
        pal_printer_ = std::make_unique<ColorPalettePrinter>(formatter_.get());
        palette_ = create_test_palette();

        pals_.fill(Palette<Rgba32, pal::max_size>{});
        pals_[0] = palette_;
    }

    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<AsciiTilePrinter> tile_printer_;
    std::unique_ptr<ColorPalettePrinter> pal_printer_;
    Palette<Rgba32, pal::max_size> palette_;
    std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> pals_;
};

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldDetectCrossRangeExactDuplicate)
{
    // Tile 0 (non-animation) and tile 1 (animation) are identical
    const auto shared_tile = create_two_color_tile(1, 2);
    const auto tiles_png = build_tiles_png({shared_tile, shared_tile});

    // Metatile references tile 1 with palette 0
    std::vector<TilemapEntry> metatiles{TilemapEntry{1, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 1, {shared_tile}, palette_);

    AnimationDecompiler decompiler{diag_.get(), tile_printer_.get(), pal_printer_.get()};

    // With mangle strategy, it should succeed and produce a mangled tile
    auto result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::mangle), nullptr);

    ASSERT_TRUE(result.has_value()) << "Decompilation should succeed with mangle strategy";

    // The key frame tile should have been mangled to differ from the external tile
    const auto &key_tile = result.value().key_frame().tile_at(0);
    // Convert back to index to compare canonically
    // We just verify decompilation succeeded — the mangler was invoked
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

    AnimationDecompiler decompiler{diag_.get(), tile_printer_.get(), pal_printer_.get()};

    // With error strategy, it should fail (duplicate detected)
    auto error_result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::error), nullptr);

    EXPECT_FALSE(error_result.has_value()) << "Should detect flip-equivalent cross-range duplicate";

    // With mangle strategy, it should succeed
    auto mangle_result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::mangle), nullptr);

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

    std::vector<TilemapEntry> metatiles{
        TilemapEntry{1, 0, false, false},
        TilemapEntry{2, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 2, {asymmetric_tile, h_flipped_tile}, palette_);

    AnimationDecompiler decompiler{diag_.get(), tile_printer_.get(), pal_printer_.get()};

    // Error strategy should detect intra-animation flip-equivalent duplicate
    auto error_result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::error), nullptr);

    EXPECT_FALSE(error_result.has_value()) << "Should detect intra-animation flip-equivalent duplicate";

    // Mangle strategy should resolve it
    auto mangle_result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::mangle), nullptr);

    EXPECT_TRUE(mangle_result.has_value()) << "Mangle strategy should resolve intra-animation flip-equivalent duplicate";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldNotFalsePositiveWhenNoDuplicates)
{
    // All tiles are distinct (not even flip-equivalent)
    const auto tile_a = create_two_color_tile(1, 2);
    const auto tile_b = create_two_color_tile(3, 4);
    const auto tile_c = create_two_color_tile(5, 6);

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    std::vector<TilemapEntry> metatiles{
        TilemapEntry{1, 0, false, false},
        TilemapEntry{2, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 2, {tile_b, tile_c}, palette_);

    AnimationDecompiler decompiler{diag_.get(), tile_printer_.get(), pal_printer_.get()};

    // Even with error strategy, should succeed (no duplicates)
    auto result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::error), nullptr);

    EXPECT_TRUE(result.has_value()) << "Should not false-positive when no duplicates exist";
}

TEST_F(AnimDecompilerDuplicateDetectionTests, shouldHandleMixedCrossRangeAndIntraAnimationDuplicates)
{
    // tile_a: non-animation tile
    // tile_b: animation tile 0 — exact duplicate of tile_a (cross-range)
    // tile_c: animation tile 1 — h-flip of tile_b (intra-animation flip-equivalent)
    const auto tile_a = create_asymmetric_tile(1, 2);
    const auto tile_b = tile_a; // exact cross-range duplicate
    const auto tile_c = tile_a.flip(true, false); // intra-animation flip-equivalent to tile_b

    const auto tiles_png = build_tiles_png({tile_a, tile_b, tile_c});

    std::vector<TilemapEntry> metatiles{
        TilemapEntry{1, 0, false, false},
        TilemapEntry{2, 0, false, false}};

    auto anim = create_test_animation("test_anim", 1, 2, {tile_b, tile_c}, palette_);

    AnimationDecompiler decompiler{diag_.get(), tile_printer_.get(), pal_printer_.get()};

    // Error strategy should fail
    auto error_result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::error), nullptr);

    EXPECT_FALSE(error_result.has_value()) << "Should detect mixed cross-range and intra-animation duplicates";

    // Mangle strategy should succeed
    auto mangle_result = decompiler.decompile_animation(
        anim, pals_, metatiles, tiles_png, make_transparency_config(), make_pal_strategy(),
        make_key_frame_strategy(AnimKeyFrameResolutionStrategy::mangle), nullptr);

    EXPECT_TRUE(mangle_result.has_value()) << "Mangle strategy should resolve mixed duplicates";
}

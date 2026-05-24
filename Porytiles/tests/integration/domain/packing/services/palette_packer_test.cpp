#include "gtest/gtest.h"

#include <array>
#include <bitset>
#include <set>
#include <vector>

#include "porytiles/domain/models/color_index_map.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/packing/services/best_fusion_strategy.hpp"
#include "porytiles/domain/packing/services/palette_packer.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/infra/services/color_palette_printer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles;

namespace {

[[nodiscard]] PixelTile<Rgba32> make_solid_tile(Rgba32 color)
{
    std::array<Rgba32, tile::size_pix> pixels{};
    pixels.fill(color);
    return PixelTile<Rgba32>{pixels};
}

/**
 * @brief Creates an 8x8 pixel tile with the specified colors distributed across pixels.
 *
 * @details
 * Colors are assigned to pixels in a round-robin fashion. For example, if 4 colors are provided,
 * pixel 0 gets color 0, pixel 1 gets color 1, ..., pixel 4 gets color 0 again, etc.
 *
 * @param colors The colors to distribute across the tile (must not be empty)
 * @return A PixelTile<Rgba32> with colors distributed across all pixels
 */
[[nodiscard]] PixelTile<Rgba32> make_tile_with_colors(const std::vector<Rgba32> &colors)
{
    std::array<Rgba32, tile::size_pix> pixels{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        pixels[i] = colors[i % colors.size()];
    }
    return PixelTile<Rgba32>{pixels};
}

[[nodiscard]] std::bitset<pal::num_pals> all_palettes_available()
{
    std::bitset<pal::num_pals> available{};
    available.set(); // Set all bits to 1
    return available;
}

template <typename... Indices>
[[nodiscard]] std::bitset<pal::num_pals> set_palettes_available(Indices... indices)
{
    std::bitset<pal::num_pals> available{};
    (available.set(static_cast<std::size_t>(indices)), ...);
    return available;
}

[[nodiscard]] std::set<Rgba32> collect_palette_colors(const Palette<Rgba32, pal::max_size> &pal, Rgba32 transparency)
{
    std::set<Rgba32> colors{};
    for (std::size_t i = 0; i < pal::max_size; ++i) {
        if (!pal.is_wildcard(i)) {
            const auto &color = pal.at(i);
            if (!color.is_transparent(transparency)) {
                colors.insert(color);
            }
        }
    }
    return colors;
}

[[nodiscard]] std::vector<Rgba32> generate_distinct_colors(std::size_t count)
{
    std::vector<Rgba32> colors{};
    colors.reserve(count);

    // Generate colors by incrementing through RGB space, avoiding magenta (255,0,255)
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    while (colors.size() < count) {
        Rgba32 color{r, g, b};
        // Skip magenta (transparency color)
        if (!(r == 255 && g == 0 && b == 255)) {
            colors.push_back(color);
        }

        // Increment through color space
        b += 17; // Step by 17 to get ~15 values per channel
        if (b < 17) {
            g += 17;
            if (g < 17) {
                r += 17;
            }
        }
    }

    return colors;
}

} // namespace

TEST(PalettePackerIntegration, EmptyInput_ReturnsEmptyResult)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    PackingParams params{};
    params.tiles_ = {};
    params.color_map_ = ColorIndexMap<Rgba32>{};
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().tile_to_pal_.empty());
}

TEST(PalettePackerIntegration, SingleTileOneColor_PacksIntoOnePalette)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    std::vector<PixelTile<Rgba32>> tiles{make_solid_tile(rgba_red)};
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Tile 0 should be assigned to some palette
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    const std::size_t assigned_pal = packing.tile_to_pal_.at(0);

    // That palette should exist and contain red
    ASSERT_TRUE(packing.pals_[assigned_pal].has_value());
    const auto colors = collect_palette_colors(packing.pals_[assigned_pal].value(), rgba_magenta);
    EXPECT_TRUE(colors.contains(rgba_red));
}

TEST(PalettePackerIntegration, SingleTileMaxColors_PacksIntoOnePalette)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Generate 15 distinct colors (max per palette, excluding transparency slot)
    const auto distinct_colors = generate_distinct_colors(15);
    std::vector<PixelTile<Rgba32>> tiles{make_tile_with_colors(distinct_colors)};
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Tile 0 should be assigned
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    const std::size_t assigned_pal = packing.tile_to_pal_.at(0);

    // That palette should contain all 15 colors
    ASSERT_TRUE(packing.pals_[assigned_pal].has_value());
    const auto colors = collect_palette_colors(packing.pals_[assigned_pal].value(), rgba_magenta);
    EXPECT_EQ(colors.size(), 15);
    for (const auto &expected_color : distinct_colors) {
        EXPECT_TRUE(colors.contains(expected_color)) << "Missing color: " << expected_color;
    }
}

TEST(PalettePackerIntegration, TwoTilesIdenticalColors_SharePalette)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    std::vector<PixelTile<Rgba32>> tiles{
        make_tile_with_colors({rgba_red, rgba_blue}), make_tile_with_colors({rgba_red, rgba_blue})};
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Both tiles should be assigned
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    ASSERT_TRUE(packing.tile_to_pal_.contains(1));

    // Both tiles should share the same palette
    EXPECT_EQ(packing.tile_to_pal_.at(0), packing.tile_to_pal_.at(1));
}

TEST(PalettePackerIntegration, TwoTilesDisjointColorsFitTogether_PacksSuccessfully)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Tile A: 7 colors, Tile B: 8 different colors (total 15, fits in one palette)
    const auto colors_a = generate_distinct_colors(7);
    auto colors_b = generate_distinct_colors(15);
    // Take the last 8 colors to ensure they're different from colors_a
    colors_b.erase(colors_b.begin(), colors_b.begin() + 7);

    std::vector<PixelTile<Rgba32>> tiles{make_tile_with_colors(colors_a), make_tile_with_colors(colors_b)};
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Both tiles should be assigned
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    ASSERT_TRUE(packing.tile_to_pal_.contains(1));

    // Note: The Best Fusion algorithm may or may not merge these tiles into the same palette
    // depending on weighted cost calculations. We just verify both tiles are assigned.
    // Collect all colors from both assigned palettes
    std::set<Rgba32> all_packed_colors{};
    const std::size_t pal_0 = packing.tile_to_pal_.at(0);
    const std::size_t pal_1 = packing.tile_to_pal_.at(1);

    ASSERT_TRUE(packing.pals_[pal_0].has_value());
    auto colors_pal_0 = collect_palette_colors(packing.pals_[pal_0].value(), rgba_magenta);
    all_packed_colors.insert(colors_pal_0.begin(), colors_pal_0.end());

    if (pal_0 != pal_1) {
        ASSERT_TRUE(packing.pals_[pal_1].has_value());
        auto colors_pal_1 = collect_palette_colors(packing.pals_[pal_1].value(), rgba_magenta);
        all_packed_colors.insert(colors_pal_1.begin(), colors_pal_1.end());
    }

    // All 15 colors should be present across the assigned palette(s)
    EXPECT_EQ(all_packed_colors.size(), 15);
}

TEST(PalettePackerIntegration, NoAvailablePalettes_Fails)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    std::vector<PixelTile<Rgba32>> tiles{make_solid_tile(rgba_red)};
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = std::bitset<pal::num_pals>{}; // No palettes available

    auto result = packer.pack_tiles(params);

    EXPECT_FALSE(result.has_value());
}

TEST(PalettePackerIntegration, AllPalettesNeeded_Success)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Create 16 tiles, each with 15 unique colors (no overlap)
    std::vector<PixelTile<Rgba32>> tiles{};
    for (std::size_t i = 0; i < 16; ++i) {
        // Generate unique colors for each tile by using different starting points
        std::vector<Rgba32> tile_colors{};
        for (std::size_t j = 0; j < 15; ++j) {
            // Create unique colors: use tile index and color index to generate RGB
            const auto r = static_cast<std::uint8_t>((i * 16 + j) % 256);
            const auto g = static_cast<std::uint8_t>((i * 17 + j * 3) % 256);
            const auto b = static_cast<std::uint8_t>((i * 19 + j * 7) % 256);
            // Skip if it's magenta
            if (r == 255 && g == 0 && b == 255) {
                tile_colors.emplace_back(r, g + 1, b); // Adjust to avoid magenta
            }
            else {
                tile_colors.emplace_back(r, g, b);
            }
        }
        tiles.push_back(make_tile_with_colors(tile_colors));
    }

    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // All 16 tiles should be assigned
    EXPECT_EQ(packing.tile_to_pal_.size(), 16);

    // Count how many palettes are used
    std::set<std::size_t> used_palettes{};
    for (const auto &[tile_idx, pal_idx] : packing.tile_to_pal_) {
        used_palettes.insert(pal_idx);
    }

    // All 16 palettes should be used (since colors don't overlap)
    EXPECT_EQ(used_palettes.size(), 16);
}

TEST(PalettePackerIntegration, AlmostFullPrefilledPalette_TilesGoElsewhere)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Generate 13 colors for the prefilled palette (leaves 2 slots available)
    const auto prefilled_colors = generate_distinct_colors(13);

    // Create the tile with 5 colors that are NOT in the prefilled palette
    // This tile's 5 colors won't fit in the 2 available slots of palette 0
    std::vector<Rgba32> tile_colors{};
    for (int i = 0; i < 5; ++i) {
        // Use colors that won't be in the first 13 generated
        tile_colors.emplace_back(200 + i, 200, 200);
    }
    std::vector<PixelTile<Rgba32>> tiles{make_tile_with_colors(tile_colors)};

    // Prefill palette 0 with 13 colors (2 slots remaining after slot 0)
    Palette<Rgba32, pal::max_size> prefilled_pal{};
    prefilled_pal.set(0, rgba_magenta); // Slot 0 is transparency
    for (std::size_t i = 0; i < 13; ++i) {
        prefilled_pal.set(i + 1, prefilled_colors[i]);
    }
    // Slots 14-15 are wildcards

    // Create ColorIndexMap from tiles, then add prefilled palette colors
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }
    color_map.add_pal(prefilled_pal, rgba_magenta);

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals[0] = prefilled_pal;

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = prefilled_pals;
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Tile should be assigned to a palette other than 0 (since 5 colors > 2 available slots)
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    EXPECT_NE(packing.tile_to_pal_.at(0), 0) << "Tile should not be assigned to palette 0 (not enough room)";
}

TEST(PalettePackerIntegration, PartiallyPrefilledPalette_TileCanMerge)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Prefill palette 0 with 5 colors (partially locked)
    // Colors: red, green, blue, yellow, cyan (slots 1-5)
    Palette<Rgba32, pal::max_size> prefilled_pal{};
    prefilled_pal.set(0, rgba_magenta); // Slot 0 is transparency
    prefilled_pal.set(1, rgba_red);
    prefilled_pal.set(2, rgba_green);
    prefilled_pal.set(3, rgba_blue);
    prefilled_pal.set(4, rgba_yellow);
    prefilled_pal.set(5, rgba_cyan);
    // Slots 6-15 are wildcards (default)

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals[0] = prefilled_pal;

    // Create a tile with some overlapping colors (red, blue) plus new colors (purple, lime)
    std::vector<PixelTile<Rgba32>> tiles{make_tile_with_colors({rgba_red, rgba_blue, rgba_purple, rgba_lime})};

    // Create ColorIndexMap from tiles, then add prefilled palette colors
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }
    color_map.add_pal(prefilled_pal, rgba_magenta);

    PackingParams params{};
    params.tiles_ = tiles; // Only pack the actual tile
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = prefilled_pals;
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Tile should be assigned
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    const std::size_t assigned_pal = packing.tile_to_pal_.at(0);

    // Check that palette 0 was used (it had room for the new colors)
    EXPECT_EQ(assigned_pal, 0) << "Tile should be merged into partially filled palette 0";

    // Verify the prefilled colors are still in their original slots
    ASSERT_TRUE(packing.pals_[0].has_value());
    const auto &final_pal = packing.pals_[0].value();
    EXPECT_EQ(final_pal.at(1), rgba_red);
    EXPECT_EQ(final_pal.at(2), rgba_green);
    EXPECT_EQ(final_pal.at(3), rgba_blue);
    EXPECT_EQ(final_pal.at(4), rgba_yellow);
    EXPECT_EQ(final_pal.at(5), rgba_cyan);

    // Verify the new colors are present somewhere in the palette
    const auto colors = collect_palette_colors(final_pal, rgba_magenta);
    EXPECT_TRUE(colors.contains(rgba_purple));
    EXPECT_TRUE(colors.contains(rgba_lime));
}

TEST(PalettePackerIntegration, OutOfBandPrefilledPaletteNotUsed)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Prefill palette 7 with 5 colors (partially locked)
    // Colors: red, green, blue, yellow, cyan (slots 1-5)
    Palette<Rgba32, pal::max_size> prefilled_pal{};
    prefilled_pal.set(0, rgba_magenta); // Slot 0 is transparency
    prefilled_pal.set(1, rgba_red);
    prefilled_pal.set(2, rgba_green);
    prefilled_pal.set(3, rgba_blue);
    prefilled_pal.set(4, rgba_yellow);
    prefilled_pal.set(5, rgba_cyan);
    // Slots 6-15 are wildcards (default)

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals[7] = prefilled_pal;

    // Create a tile with some overlapping colors (red, blue) plus new colors (purple, lime)
    std::vector<PixelTile<Rgba32>> tiles{make_tile_with_colors({rgba_red, rgba_blue, rgba_purple, rgba_lime})};

    // Create ColorIndexMap from tiles, then add prefilled palette colors
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }
    color_map.add_pal(prefilled_pal, rgba_magenta);

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = prefilled_pals;
    params.hints_ = {};
    // Only palettes 0-6 are available (palette 7 is NOT available)
    params.available_pals_ = set_palettes_available(0, 1, 2, 3, 4, 5, 6);

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Tile should be assigned
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    const std::size_t assigned_pal = packing.tile_to_pal_.at(0);

    // Tile should NOT be assigned to palette 7 (it's not available)
    EXPECT_NE(assigned_pal, 7) << "Tile should not use unavailable palette 7";

    // Palette 7 should NOT be in the result (it wasn't available for packing)
    // The packer only includes palettes that are in available_pals
    EXPECT_FALSE(packing.pals_[7].has_value()) << "Unavailable palette 7 should not appear in result";
}

TEST(PalettePackerIntegration, PrefilledPaletteWithDuplicateColors_CapacityCorrectlyCalculated)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Create a prefilled palette with 15 slots filled but only 14 unique colors (1 duplicate).
    // This tests the fix for the bug where duplicate colors caused capacity miscalculation.
    // Slots 1-14 get distinct colors, slot 15 duplicates slot 1's color.
    Palette<Rgba32, pal::max_size> prefilled_pal{};
    prefilled_pal.set(0, rgba_magenta); // Slot 0 is transparency

    const auto distinct_colors = generate_distinct_colors(14);
    for (std::size_t i = 0; i < 14; ++i) {
        prefilled_pal.set(i + 1, distinct_colors[i]);
    }
    // Slot 15 duplicates slot 1's color, creating 15 occupied slots but only 14 unique colors
    prefilled_pal.set(15, distinct_colors[0]);

    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals[0] = prefilled_pal;

    // Create a tile with a brand new color not in the prefilled palette
    Rgba32 new_color{254, 253, 252}; // A color definitely not in generate_distinct_colors
    std::vector<PixelTile<Rgba32>> tiles{make_solid_tile(new_color)};

    // Create ColorIndexMap from tiles, then add prefilled palette colors
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }
    color_map.add_pal(prefilled_pal, rgba_magenta);

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = prefilled_pals;
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value()) << "Packing should succeed";
    const auto &packing = result.value();

    // Tile should be assigned
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    const std::size_t assigned_pal = packing.tile_to_pal_.at(0);

    // The tile should be assigned to a palette OTHER than 0
    // because palette 0 has no room (15 slots occupied, even though only 14 unique colors)
    EXPECT_NE(assigned_pal, 0)
        << "Tile should NOT be assigned to palette 0 (no available slots due to duplicates occupying all 15 slots)";
}

TEST(PalettePackerIntegration, TransparentPixelsIgnored_OnlyNonTransparentPacked)
{
    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    BestFusionStrategy strategy{};
    AsciiTilePrinter tile_printer{&formatter};
    ColorPalettePrinter pal_printer{&formatter};
    PalettePacker packer{&strategy, &formatter, &diag, &tile_printer, &pal_printer};

    // Create a tile with magenta (transparent) and red pixels
    // Half the tile is magenta, half is red
    std::array<Rgba32, tile::size_pix> pixels{};
    for (std::size_t i = 0; i < tile::size_pix / 2; ++i) {
        pixels[i] = rgba_magenta; // Transparent
    }
    for (std::size_t i = tile::size_pix / 2; i < tile::size_pix; ++i) {
        pixels[i] = rgba_red; // Non-transparent
    }
    std::vector<PixelTile<Rgba32>> tiles{PixelTile<Rgba32>{pixels}};
    ColorIndexMap<Rgba32> color_map{};
    for (const auto &tile : tiles) {
        color_map.add_tile(tile, rgba_magenta);
    }

    PackingParams params{};
    params.tiles_ = tiles;
    params.color_map_ = color_map;
    params.extrinsic_transparency_ = rgba_magenta;
    params.prefilled_pals_ = {};
    params.hints_ = {};
    params.available_pals_ = all_palettes_available();

    auto result = packer.pack_tiles(params);

    ASSERT_TRUE(result.has_value());
    const auto &packing = result.value();

    // Tile should be assigned
    ASSERT_TRUE(packing.tile_to_pal_.contains(0));
    const std::size_t assigned_pal = packing.tile_to_pal_.at(0);

    // The palette should contain red (the only non-transparent tile color)
    // Note: Empty palette slots are filled with Rgba32{0,0,0} by the packer,
    // so we check that the actual tile colors are present, not the count
    ASSERT_TRUE(packing.pals_[assigned_pal].has_value());
    const auto colors = collect_palette_colors(packing.pals_[assigned_pal].value(), rgba_magenta);

    // Red should be present (it was a non-transparent tile color)
    EXPECT_TRUE(colors.contains(rgba_red));

    // Magenta should NOT be present as a color (it was transparent and filtered out)
    EXPECT_FALSE(colors.contains(rgba_magenta));

    // The ColorIndexMap should only contain red (1 color), since magenta was filtered as transparent
    EXPECT_EQ(color_map.size(), 1);
}

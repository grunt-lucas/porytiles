#include <gtest/gtest.h>

#include "porytiles2/domain/models/color_index_map.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/algorithms/palette_builder.hpp"
#include "porytiles2/domain/packing/models/packed_palette.hpp"

using namespace porytiles2;

namespace {

const Rgba32 transparent = rgba_magenta;
const Rgba32 red{255, 0, 0, Rgba32::alpha_opaque};
const Rgba32 blue{0, 0, 255, Rgba32::alpha_opaque};
const Rgba32 green{0, 255, 0, Rgba32::alpha_opaque};
const Rgba32 yellow{255, 255, 0, Rgba32::alpha_opaque};
const Rgba32 white{255, 255, 255, Rgba32::alpha_opaque};
const Rgba32 orange{255, 128, 0, Rgba32::alpha_opaque};

/**
 * @brief Helper to build a ColorIndexMap from a set of colors.
 */
ColorIndexMap<Rgba32> make_color_map(const std::vector<Rgba32> &colors)
{
    ColorIndexMap<Rgba32> map;
    for (const auto &c : colors) {
        PixelTile<Rgba32> tile{transparent};
        tile.set(0, 0, c);
        map.add_tile(tile, transparent);
    }
    return map;
}

/**
 * @brief Helper to create a PackedPalette with specific colors from a color map.
 */
PackedPalette
make_packed_pal(std::size_t hw_index, const std::vector<Rgba32> &colors, const ColorIndexMap<Rgba32> &color_map)
{
    PackedPalette pal{hw_index};
    for (const auto &c : colors) {
        auto idx = color_map.index_at_color(c);
        if (idx.has_value()) {
            ColorSet cs;
            cs.set(idx.value());
            // Build a single-color tile to add
            PackableTile tile{PackableTile::RegularId{0}, cs};
            pal.add_tile(tile);
        }
    }
    return pal;
}

} // namespace

TEST(PaletteBuilderTests, NoLinks_ManualModeEquivalence)
{
    // Without Indirect links, palette builder should produce sequential fill identical to manual mode
    auto color_map = make_color_map({red, blue, green});

    PackedPalette packed0{0};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(red).value());
        cs.set(color_map.index_at_color(blue).value());
        PackableTile tile{PackableTile::RegularId{0}, cs};
        packed0.add_tile(tile);
    }

    std::vector<PackedPalette> packed_pals = {packed0};
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    std::vector<IndirectLink> no_links;

    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, no_links);

    ASSERT_TRUE(result.at(0).has_value());
    const auto &pal = result.at(0).value();

    // Slot 0 should be the default slot zero (transparent)
    EXPECT_EQ(pal.at(0), transparent);
    // for_each_color iterates by ColorIndex bit order. ColorIndexMap assigns indices via std::set ordering
    // (lexicographic on Rgba32). blue (0,0,255) sorts before red (255,0,0), so blue gets a lower index.
    EXPECT_EQ(pal.at(1), blue);
    EXPECT_EQ(pal.at(2), red);
}

TEST(PaletteBuilderTests, SimpleIndirectLink_ColorsAligned)
{
    auto color_map = make_color_map({red, blue});

    // Two palettes: pal 0 has red, pal 1 has blue
    PackedPalette packed0{0};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(red).value());
        PackableTile tile{PackableTile::RegularId{0}, cs};
        packed0.add_tile(tile);
    }

    PackedPalette packed1{1};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(blue).value());
        PackableTile tile{PackableTile::RegularId{1}, cs};
        packed1.add_tile(tile);
    }

    std::vector<PackedPalette> packed_pals = {packed0, packed1};
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};

    // Link: blue in pal 1 follows red in pal 0
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_pal = 1,
            .source_color = blue,
            .ref_pal = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, links);

    ASSERT_TRUE(result.at(0).has_value());
    ASSERT_TRUE(result.at(1).has_value());

    const auto &pal0 = result.at(0).value();
    const auto &pal1 = result.at(1).value();

    // Red in pal 0 gets sequential fill → slot 1
    EXPECT_EQ(pal0.at(1), red);
    // Blue in pal 1 should follow red → also slot 1
    EXPECT_EQ(pal1.at(1), blue);
}

TEST(PaletteBuilderTests, PrefilledSlots_Preserved)
{
    auto color_map = make_color_map({red, blue, green});

    PackedPalette packed0{0};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(red).value());
        cs.set(color_map.index_at_color(blue).value());
        PackableTile tile{PackableTile::RegularId{0}, cs};
        packed0.add_tile(tile);
    }

    std::vector<PackedPalette> packed_pals = {packed0};

    // Prefilled: green locked at slot 1 in pal 0 (default-constructed = all wildcards, then set specific slots)
    Palette<Rgba32, pal::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(1, green);
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals.at(0) = prefilled;

    std::vector<IndirectLink> no_links;

    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, no_links);

    ASSERT_TRUE(result.at(0).has_value());
    const auto &pal = result.at(0).value();

    // Slot 1 should be green (prefilled, locked)
    EXPECT_EQ(pal.at(1), green);
    // Blue sorts before red lexicographically, so blue gets placed first
    EXPECT_EQ(pal.at(2), blue);
    EXPECT_EQ(pal.at(3), red);
}

TEST(PaletteBuilderTests, IndirectLinkConflictWithPrefilled_Skipped)
{
    auto color_map = make_color_map({red, blue, green});

    PackedPalette packed0{0};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(red).value());
        PackableTile tile{PackableTile::RegularId{0}, cs};
        packed0.add_tile(tile);
    }

    PackedPalette packed1{1};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(blue).value());
        PackableTile tile{PackableTile::RegularId{1}, cs};
        packed1.add_tile(tile);
    }

    std::vector<PackedPalette> packed_pals = {packed0, packed1};

    // Prefilled: green locked at slot 1 in pal 1 (default-constructed = all wildcards)
    Palette<Rgba32, pal::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(1, green);
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals.at(1) = prefilled;

    // Link: blue in pal 1 follows red in pal 0 → red gets slot 1 → blue wants slot 1 but it's prefilled
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_pal = 1,
            .source_color = blue,
            .ref_pal = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, links);

    ASSERT_TRUE(result.at(1).has_value());
    const auto &pal1 = result.at(1).value();

    // Slot 1 in pal 1 should still be green (prefilled wins)
    EXPECT_EQ(pal1.at(1), green);
}

TEST(PaletteBuilderTests, PrefilledSourceColor_LinkDropped_CounterIncremented)
{
    // green is prefilled (locked) in palette 1 at slot 2, so any link targeting green as source_color
    // should be dropped in Phase 2 and counted as prefilled_source_conflict
    auto color_map = make_color_map({red, blue, green});

    // Palette 0 has red
    PackedPalette packed0{0};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(red).value());
        PackableTile tile{PackableTile::RegularId{0}, cs};
        packed0.add_tile(tile);
    }

    // Palette 1 has blue and green
    PackedPalette packed1{1};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(blue).value());
        cs.set(color_map.index_at_color(green).value());
        PackableTile tile{PackableTile::RegularId{1}, cs};
        packed1.add_tile(tile);
    }

    std::vector<PackedPalette> packed_pals = {packed0, packed1};

    // Prefilled: green locked at slot 2 in palette 1
    Palette<Rgba32, pal::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(2, green);
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals.at(1) = prefilled;

    // Link: green in pal 1 follows red in pal 0 — but green is prefilled, so this should be dropped
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_pal = 1,
            .source_color = green,
            .ref_pal = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, links, &fc);

    // The prefilled source conflict detail should be recorded
    EXPECT_EQ(fc.prefilled_source_conflict_details.size(), 1u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).source_group_index, 0u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).source_pal_index, 1u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).source_color, green);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).ref_pal_index, 0u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).ref_color, red);

    // Other detail vectors should remain empty
    EXPECT_EQ(fc.first_writer_wins_details.size(), 0u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.size(), 0u);
    EXPECT_EQ(fc.broken_chain_details.size(), 0u);
    EXPECT_EQ(fc.no_free_slot_details.size(), 0u);

    // Total should include the new counter
    EXPECT_EQ(fc.total(), 1u);

    // Green should still be at its prefilled slot (slot 2) in palette 1
    ASSERT_TRUE(result.at(1).has_value());
    EXPECT_EQ(result.at(1).value().at(2), green);
}

TEST(PaletteBuilderTests, PrefilledSourceColor_NaturallyAligned_NoConflictRecorded)
{
    // Both source (blue, pal 1) and ref (red, pal 0) are prefilled at slot 4.
    // Link: blue→red. Since both are AbsolutePosition at the same slot, alignment is
    // naturally satisfied — no prefilled source conflict should be recorded.
    auto color_map = make_color_map({red, blue});

    // Palette 0 has red
    PackedPalette packed0{0};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(red).value());
        PackableTile tile{PackableTile::RegularId{0}, cs};
        packed0.add_tile(tile);
    }

    // Palette 1 has blue
    PackedPalette packed1{1};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(blue).value());
        PackableTile tile{PackableTile::RegularId{1}, cs};
        packed1.add_tile(tile);
    }

    std::vector<PackedPalette> packed_pals = {packed0, packed1};

    // Prefilled: red locked at slot 4 in palette 0, blue locked at slot 4 in palette 1
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    {
        Palette<Rgba32, pal::max_size> prefilled0{};
        prefilled0.set(0, transparent);
        prefilled0.set(4, red);
        prefilled_pals.at(0) = prefilled0;
    }
    {
        Palette<Rgba32, pal::max_size> prefilled1{};
        prefilled1.set(0, transparent);
        prefilled1.set(4, blue);
        prefilled_pals.at(1) = prefilled1;
    }

    // Link: blue in pal 1 follows red in pal 0
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_pal = 1,
            .source_color = blue,
            .ref_pal = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, links, &fc);

    // No conflicts should be recorded — alignment is naturally satisfied
    EXPECT_EQ(fc.prefilled_source_conflict_details.size(), 0u);
    EXPECT_EQ(fc.first_writer_wins_details.size(), 0u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.size(), 0u);
    EXPECT_EQ(fc.broken_chain_details.size(), 0u);
    EXPECT_EQ(fc.no_free_slot_details.size(), 0u);
    EXPECT_EQ(fc.total(), 0u);

    // Red should be at slot 4 in palette 0
    ASSERT_TRUE(result.at(0).has_value());
    EXPECT_EQ(result.at(0).value().at(4), red);

    // Blue should be at slot 4 in palette 1
    ASSERT_TRUE(result.at(1).has_value());
    EXPECT_EQ(result.at(1).value().at(4), blue);
}

TEST(PaletteBuilderTests, PrefilledDestinationConflict_CounterIncremented_FallbackPlacement)
{
    // blue in pal 1 links to red in pal 0 (shape group 0). Red gets slot 1 via sequential fill.
    // Green is prefilled at slot 1 in pal 1, so resolving blue → slot 1 hits a destination conflict.
    // Phase 5 fallback should assign blue to the next free slot (slot 2).
    auto color_map = make_color_map({red, blue, green});

    // Palette 0 has red (no prefilled)
    PackedPalette packed0{0};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(red).value());
        PackableTile tile{PackableTile::RegularId{0}, cs};
        packed0.add_tile(tile);
    }

    // Palette 1 has blue (green is prefilled, not packed)
    PackedPalette packed1{1};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(blue).value());
        PackableTile tile{PackableTile::RegularId{1}, cs};
        packed1.add_tile(tile);
    }

    std::vector<PackedPalette> packed_pals = {packed0, packed1};

    // Prefilled: green locked at slot 1 in palette 1
    Palette<Rgba32, pal::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(1, green);
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    prefilled_pals.at(1) = prefilled;

    // Link: blue in pal 1 follows red in pal 0
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_pal = 1,
            .source_color = blue,
            .ref_pal = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, links, &fc);

    // Exactly one prefilled destination conflict should be recorded
    ASSERT_EQ(fc.prefilled_destination_conflict_details.size(), 1u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.at(0).source_group_index, 0u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.at(0).palette_index, 1u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.at(0).target_slot, 1u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.at(0).blocked_color, blue);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.at(0).locked_color, green);

    // All other detail vectors should remain empty
    EXPECT_EQ(fc.prefilled_source_conflict_details.size(), 0u);
    EXPECT_EQ(fc.first_writer_wins_details.size(), 0u);
    EXPECT_EQ(fc.broken_chain_details.size(), 0u);
    EXPECT_EQ(fc.no_free_slot_details.size(), 0u);

    // Total should be exactly 1
    EXPECT_EQ(fc.total(), 1u);

    // Palette 0: red at slot 1 (sequential fill, unaffected)
    ASSERT_TRUE(result.at(0).has_value());
    EXPECT_EQ(result.at(0).value().at(1), red);

    // Palette 1: green preserved at prefilled slot 1, blue placed at fallback slot 2
    ASSERT_TRUE(result.at(1).has_value());
    EXPECT_EQ(result.at(1).value().at(1), green);
    EXPECT_EQ(result.at(1).value().at(2), blue);
}

TEST(PaletteBuilderTests, EmptyPackedPals_ReturnsAllNullopt)
{
    auto color_map = make_color_map({red});
    std::vector<PackedPalette> packed_pals;
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> prefilled_pals{};
    std::vector<IndirectLink> no_links;

    auto result = build_all_output_palettes(packed_pals, prefilled_pals, color_map, transparent, no_links);

    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        EXPECT_FALSE(result.at(i).has_value());
    }
}

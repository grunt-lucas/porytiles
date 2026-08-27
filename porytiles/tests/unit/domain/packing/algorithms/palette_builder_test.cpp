#include <gtest/gtest.h>

#include "porytiles/domain/models/color_index_map.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/packing/algorithms/palette_builder.hpp"
#include "porytiles/domain/packing/models/packed_palette.hpp"

using namespace porytiles;

namespace {

const Rgba32 transparent = rgba_magenta;
const Rgba32 red{255, 0, 0, Rgba32::alpha_opaque};
const Rgba32 blue{0, 0, 255, Rgba32::alpha_opaque};
const Rgba32 green{0, 255, 0, Rgba32::alpha_opaque};
const Rgba32 yellow{255, 255, 0, Rgba32::alpha_opaque};
const Rgba32 white{255, 255, 255, Rgba32::alpha_opaque};
const Rgba32 orange{255, 128, 0, Rgba32::alpha_opaque};

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

PackedPalette
make_packed_palette(std::size_t hw_index, const std::vector<Rgba32> &colors, const ColorIndexMap<Rgba32> &color_map)
{
    PackedPalette palette{hw_index};
    for (const auto &c : colors) {
        auto idx = color_map.index_at_color(c);
        if (idx.has_value()) {
            ColorSet cs;
            cs.set(idx.value());
            // Build a single-color tile to add
            PackableTile tile{PackableTile::RegularId{0}, cs};
            palette.add_tile(tile);
        }
    }
    return palette;
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

    std::vector<PackedPalette> packed_palettes = {packed0};
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    std::vector<IndirectLink> no_links;

    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, no_links);

    ASSERT_TRUE(result.at(0).has_value());
    const auto &palette = result.at(0).value();

    // Slot 0 should be the default slot zero (transparent)
    EXPECT_EQ(palette.at(0), transparent);
    // for_each_color iterates by ColorIndex bit order. ColorIndexMap assigns indices via std::set ordering
    // (lexicographic on Rgba32). blue (0,0,255) sorts before red (255,0,0), so blue gets a lower index.
    EXPECT_EQ(palette.at(1), blue);
    EXPECT_EQ(palette.at(2), red);
}

TEST(PaletteBuilderTests, SimpleIndirectLink_ColorsAligned)
{
    auto color_map = make_color_map({red, blue});

    // Two palettes: palette 0 has red, palette 1 has blue
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

    std::vector<PackedPalette> packed_palettes = {packed0, packed1};
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    // Link: blue in palette 1 follows red in palette 0
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 1,
            .source_color = blue,
            .ref_palette = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links);

    ASSERT_TRUE(result.at(0).has_value());
    ASSERT_TRUE(result.at(1).has_value());

    const auto &palette0 = result.at(0).value();
    const auto &palette1 = result.at(1).value();

    // Red in palette 0 gets sequential fill -> slot 1
    EXPECT_EQ(palette0.at(1), red);
    // Blue in palette 1 should follow red -> also slot 1
    EXPECT_EQ(palette1.at(1), blue);
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

    std::vector<PackedPalette> packed_palettes = {packed0};

    // Prefilled: green locked at slot 1 in palette 0 (default-constructed = all wildcards, then set specific slots)
    Palette<Rgba32, palette::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(1, green);
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    prefilled_palettes.at(0) = prefilled;

    std::vector<IndirectLink> no_links;

    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, no_links);

    ASSERT_TRUE(result.at(0).has_value());
    const auto &palette = result.at(0).value();

    // Slot 1 should be green (prefilled, locked)
    EXPECT_EQ(palette.at(1), green);
    // Blue sorts before red lexicographically, so blue gets placed first
    EXPECT_EQ(palette.at(2), blue);
    EXPECT_EQ(palette.at(3), red);
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

    std::vector<PackedPalette> packed_palettes = {packed0, packed1};

    // Prefilled: green locked at slot 1 in palette 1 (default-constructed = all wildcards)
    Palette<Rgba32, palette::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(1, green);
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    prefilled_palettes.at(1) = prefilled;

    // Link: blue in palette 1 follows red in palette 0 -> red gets slot 1 -> blue wants slot 1 but it's prefilled
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 1,
            .source_color = blue,
            .ref_palette = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links);

    ASSERT_TRUE(result.at(1).has_value());
    const auto &palette1 = result.at(1).value();

    // Slot 1 in palette 1 should still be green (prefilled wins)
    EXPECT_EQ(palette1.at(1), green);
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

    std::vector<PackedPalette> packed_palettes = {packed0, packed1};

    // Prefilled: green locked at slot 2 in palette 1
    Palette<Rgba32, palette::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(2, green);
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    prefilled_palettes.at(1) = prefilled;

    // Link: green in palette 1 follows red in palette 0, but green is prefilled, so this should be dropped
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 1,
            .source_color = green,
            .ref_palette = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links, &fc);

    // The prefilled source conflict detail should be recorded
    EXPECT_EQ(fc.prefilled_source_conflict_details.size(), 1u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).source_group_index, 0u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).source_palette_index, 1u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).source_color, green);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).ref_palette_index, 0u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.at(0).ref_color, red);

    // Other detail vectors should remain empty
    EXPECT_EQ(fc.first_writer_wins_details.size(), 0u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.size(), 0u);

    // Total should include the new counter
    EXPECT_EQ(fc.total(), 1u);

    // Green should still be at its prefilled slot (slot 2) in palette 1
    ASSERT_TRUE(result.at(1).has_value());
    EXPECT_EQ(result.at(1).value().at(2), green);
}

TEST(PaletteBuilderTests, PrefilledSourceColor_NaturallyAligned_NoConflictRecorded)
{
    // Both source (blue, palette 1) and ref (red, palette 0) are prefilled at slot 4.
    // Link: blue->red. Since both are AbsolutePosition at the same slot, alignment is
    // naturally satisfied. No prefilled source conflict should be recorded.
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

    std::vector<PackedPalette> packed_palettes = {packed0, packed1};

    // Prefilled: red locked at slot 4 in palette 0, blue locked at slot 4 in palette 1
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    {
        Palette<Rgba32, palette::max_size> prefilled0{};
        prefilled0.set(0, transparent);
        prefilled0.set(4, red);
        prefilled_palettes.at(0) = prefilled0;
    }
    {
        Palette<Rgba32, palette::max_size> prefilled1{};
        prefilled1.set(0, transparent);
        prefilled1.set(4, blue);
        prefilled_palettes.at(1) = prefilled1;
    }

    // Link: blue in palette 1 follows red in palette 0
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 1,
            .source_color = blue,
            .ref_palette = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links, &fc);

    // No conflicts should be recorded. Alignment is naturally satisfied
    EXPECT_EQ(fc.prefilled_source_conflict_details.size(), 0u);
    EXPECT_EQ(fc.first_writer_wins_details.size(), 0u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.size(), 0u);

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
    // blue in palette 1 links to red in palette 0 (shape group 0). Red gets slot 1 via sequential fill.
    // Green is prefilled at slot 1 in palette 1, so resolving blue -> slot 1 hits a destination conflict.
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

    std::vector<PackedPalette> packed_palettes = {packed0, packed1};

    // Prefilled: green locked at slot 1 in palette 1
    Palette<Rgba32, palette::max_size> prefilled{};
    prefilled.set(0, transparent);
    prefilled.set(1, green);
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    prefilled_palettes.at(1) = prefilled;

    // Link: blue in palette 1 follows red in palette 0
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 1,
            .source_color = blue,
            .ref_palette = 0,
            .ref_color = red,
            .source_group_index = 0,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links, &fc);

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

TEST(PaletteBuilderTests, CompatibleFWW_NoFalsePositive)
{
    // Two IndirectLinks target the same color (red in palette 0) with identical ref_palette/ref_color (blue in palette
    // 1). The existing IndirectPosition already satisfies both groups. No conflict should be recorded.
    auto color_map = make_color_map({red, blue});

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

    std::vector<PackedPalette> packed_palettes = {packed0, packed1};
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    // Two links from different groups, both linking red in palette 0 to blue in palette 1
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 0,
            .source_color = red,
            .ref_palette = 1,
            .ref_color = blue,
            .source_group_index = 0,
        },
        IndirectLink{
            .source_palette = 0,
            .source_color = red,
            .ref_palette = 1,
            .ref_color = blue,
            .source_group_index = 1,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links, &fc);

    // Compatible links, no conflict
    EXPECT_EQ(fc.first_writer_wins_details.size(), 0u);
    EXPECT_EQ(fc.total(), 0u);

    // Red in palette 0 should follow blue in palette 1 (Indirect alignment)
    ASSERT_TRUE(result.at(0).has_value());
    ASSERT_TRUE(result.at(1).has_value());
    EXPECT_EQ(result.at(0).value().at(1), red);
    EXPECT_EQ(result.at(1).value().at(1), blue);
}

TEST(PaletteBuilderTests, GenuineFWW_EnrichedDetail)
{
    // Two IndirectLinks target the same color (red in palette 0) with DIFFERENT references.
    // Group 0 links red->blue in palette 1, group 1 links red->green in palette 2.
    // Group 0 wins, group 1's link is dropped. Detail should capture both sides.
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

    PackedPalette packed2{2};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(green).value());
        PackableTile tile{PackableTile::RegularId{2}, cs};
        packed2.add_tile(tile);
    }

    std::vector<PackedPalette> packed_palettes = {packed0, packed1, packed2};
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    // Group 0 links red in palette 0 -> blue in palette 1 (wins)
    // Group 1 links red in palette 0 -> green in palette 2 (loses)
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 0,
            .source_color = red,
            .ref_palette = 1,
            .ref_color = blue,
            .source_group_index = 0,
        },
        IndirectLink{
            .source_palette = 0,
            .source_color = red,
            .ref_palette = 2,
            .ref_color = green,
            .source_group_index = 1,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links, &fc);

    // Exactly one genuine FWW conflict
    ASSERT_EQ(fc.first_writer_wins_details.size(), 1u);
    const auto &detail = fc.first_writer_wins_details.at(0);

    // Losing group
    EXPECT_EQ(detail.source_group_index, 1u);
    EXPECT_EQ(detail.source_palette_index, 0u);
    EXPECT_EQ(detail.source_color, red);

    // Winning side
    EXPECT_EQ(detail.winning_group_index, 0u);
    EXPECT_EQ(detail.winning_ref_palette_index, 1u);
    EXPECT_EQ(detail.winning_ref_color, blue);

    // Losing side's wanted reference
    EXPECT_EQ(detail.losing_ref_palette_index, 2u);
    EXPECT_EQ(detail.losing_ref_color, green);

    // Other failure types should be empty
    EXPECT_EQ(fc.prefilled_source_conflict_details.size(), 0u);
    EXPECT_EQ(fc.prefilled_destination_conflict_details.size(), 0u);

    EXPECT_EQ(fc.total(), 1u);
}

TEST(PaletteBuilderTests, CrossPaletteEvictionDisplacement_MismatchDetected)
{
    // Setup: Palette 0 has colors A and B. Palette 1 has colors C and D.
    // Links: A in palette 0 follows C in palette 1 (group 0), D in palette 1 follows B in palette 0 (group 1).
    //
    // Phase 3 fills sequentially: palette 0 gets B at some slot, palette 1 gets C at some slot.
    // Phase 4 processes palette 0 first: A resolves to C's slot in palette 1. Then processes palette 1:
    // D resolves to B's slot in palette 0, potentially evicting C from its original slot.
    // After Phase 5, A's slot may no longer match C's final slot -> mismatch detected.
    //
    // We use 4 colors: red, blue, green, yellow. Palette 0 has {red, blue}, palette 1 has {green, yellow}.
    // Link: red in palette 0 -> green in palette 1 (group 0). yellow in palette 1 -> blue in palette 0 (group 1).
    //
    // Phase 3: palette 0 fills blue->slot1, red stays Indirect. palette 1 fills green->slot1, yellow stays Indirect.
    // Phase 4 palette 0: red resolves to green's slot (1). Slot 1 in palette 0 is occupied by blue -> evict blue to
    // slot 2.
    //   Now palette 0: red@1, blue@2.
    // Phase 4 palette 1: yellow resolves to blue's slot. blue is now at slot 2 in palette 0.
    //   yellow wants slot 2 in palette 1. Slot 2 is free -> yellow@2. green stays @1.
    //   Result: palette 1: green@1, yellow@2.
    //
    // Final check: red@1 in palette0, green@1 in palette1 -> match (OK).
    //              yellow@2 in palette1, blue@2 in palette0 -> match (OK).
    //
    // Hmm, this scenario actually works. Let me construct one that actually causes displacement.
    // The key is that Phase 4 processes palettes sequentially, so palette 0's resolution reads palette 1's
    // pre-Phase-4 state, and then palette 1's Phase 4 eviction displaces the reference color.
    //
    // Need: palette 1 has two Indirect colors that resolve to the same slot, causing eviction of a color
    // that palette 0 already resolved against.
    //
    // Setup: 5 colors. Palette 0 has {red}. Palette 1 has {blue, green, yellow}.
    // Link: red in palette 0 -> blue in palette 1 (group 0). green in palette 1 -> yellow in palette 2 (group 1).
    // Palette 2 has {yellow, orange}.
    //
    // Actually, let me think about this more carefully. The simplest eviction displacement:
    // - Palette 0 has {red}, palette 1 has {blue, green}.
    // - Palette 2 has {yellow}.
    // - Link: red in palette 0 -> blue in palette 1 (group 0).
    // - Link: green in palette 1 -> yellow in palette 2 (group 1).
    // - Phase 3: palette 1 has no Undetermined colors (both are Indirect... wait, no).
    //
    // Let me try: both blue and green are in palette 1. blue is Undetermined (reference for red).
    // green has a link to yellow in palette 2. yellow is also in palette 2 as Undetermined.
    //
    // Phase 3: palette 1 fills blue -> slot 1 (Undetermined). green is Indirect, skipped.
    //          palette 2 fills yellow -> slot 1 (Undetermined).
    // Phase 4 palette 0: red resolves via blue in palette 1 -> blue is at slot 1 -> red placed at slot 1 in palette 0.
    // Phase 4 palette 1: green resolves via yellow in palette 2 -> yellow at slot 1 -> green wants slot 1 in palette 1.
    //          Slot 1 is occupied by blue -> evict blue to slot 2. green@1, blue@2.
    // Post-check: red@1 in palette 0, blue now @2 in palette 1 -> MISMATCH!
    const Rgba32 cyan{0, 255, 255, Rgba32::alpha_opaque};

    auto color_map = make_color_map({red, blue, green, yellow, cyan});

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

    // Palette 2 has yellow
    PackedPalette packed2{2};
    {
        ColorSet cs;
        cs.set(color_map.index_at_color(yellow).value());
        PackableTile tile{PackableTile::RegularId{2}, cs};
        packed2.add_tile(tile);
    }

    std::vector<PackedPalette> packed_palettes = {packed0, packed1, packed2};
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    // Group 0: red in palette 0 -> blue in palette 1
    // Group 1: green in palette 1 -> yellow in palette 2
    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 0,
            .source_color = red,
            .ref_palette = 1,
            .ref_color = blue,
            .source_group_index = 0,
        },
        IndirectLink{
            .source_palette = 1,
            .source_color = green,
            .ref_palette = 2,
            .ref_color = yellow,
            .source_group_index = 1,
        },
    };

    AlignmentFailureCounts fc{};
    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links, &fc);

    // Phase 3: palette 1 fills blue -> slot 1 (Undetermined). green is Indirect.
    //          palette 2 fills yellow -> slot 1.
    // Phase 4 palette 0: red resolves to blue's slot 1 -> red@1 in palette 0.
    // Phase 4 palette 1: green resolves to yellow's slot 1 -> green wants slot 1 in palette 1.
    //          blue at slot 1 -> evict blue to slot 2. Now: green@1, blue@2.
    // Post-check: red@1 in palette 0, blue@2 in palette 1 -> MISMATCH for group 0.

    // There should be exactly one post-resolution mismatch
    ASSERT_EQ(fc.post_resolution_mismatch_details.size(), 1u);
    const auto &detail = fc.post_resolution_mismatch_details.at(0);
    EXPECT_EQ(detail.source_group_index, 0u);
    EXPECT_EQ(detail.source_palette_index, 0u);
    EXPECT_EQ(detail.source_color, red);
    EXPECT_EQ(detail.source_final_slot, 1u);
    EXPECT_EQ(detail.ref_palette_index, 1u);
    EXPECT_EQ(detail.ref_color, blue);
    EXPECT_EQ(detail.ref_final_slot, 2u);

    // Other failure types should be empty

    EXPECT_EQ(fc.prefilled_destination_conflict_details.size(), 0u);
    EXPECT_EQ(fc.prefilled_source_conflict_details.size(), 0u);

    EXPECT_EQ(fc.first_writer_wins_details.size(), 0u);

    // Total should include the mismatch
    EXPECT_EQ(fc.total(), 1u);

    // Verify final palette state
    ASSERT_TRUE(result.at(0).has_value());
    ASSERT_TRUE(result.at(1).has_value());
    ASSERT_TRUE(result.at(2).has_value());
    EXPECT_EQ(result.at(0).value().at(1), red);
    EXPECT_EQ(result.at(2).value().at(1), yellow);
    // green@1, blue@2 in palette 1 (blue evicted)
    EXPECT_EQ(result.at(1).value().at(1), green);
    EXPECT_EQ(result.at(1).value().at(2), blue);
}

TEST(PaletteBuilderTests, NoDisplacement_MismatchEmpty)
{
    // Normal case: Indirect link resolves correctly and no eviction occurs.
    // red in palette 0 -> blue in palette 1. No other Indirect colors in palette 1 to cause eviction.
    auto color_map = make_color_map({red, blue});

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

    std::vector<PackedPalette> packed_palettes = {packed0, packed1};
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};

    std::vector<IndirectLink> links = {
        IndirectLink{
            .source_palette = 0,
            .source_color = red,
            .ref_palette = 1,
            .ref_color = blue,
            .source_group_index = 0,
        },
    };

    AlignmentFailureCounts fc{};
    build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, links, &fc);

    // No mismatches. Slots should match perfectly
    EXPECT_TRUE(fc.post_resolution_mismatch_details.empty());
    EXPECT_EQ(fc.total(), 0u);
}

TEST(PaletteBuilderTests, EmptyPackedPalettes_ReturnsAllNullopt)
{
    auto color_map = make_color_map({red});
    std::vector<PackedPalette> packed_palettes;
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> prefilled_palettes{};
    std::vector<IndirectLink> no_links;

    auto result = build_all_output_palettes(packed_palettes, prefilled_palettes, color_map, transparent, no_links);

    for (std::size_t i = 0; i < palette::num_palettes; ++i) {
        EXPECT_FALSE(result.at(i).has_value());
    }
}

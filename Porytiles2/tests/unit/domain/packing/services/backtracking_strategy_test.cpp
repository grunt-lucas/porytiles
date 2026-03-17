#include "gtest/gtest.h"

#include <bitset>
#include <cstddef>
#include <limits>
#include <set>
#include <variant>
#include <vector>

#include "porytiles2/domain/config/search_algorithm.hpp"
#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"
#include "porytiles2/domain/packing/models/palette_pool.hpp"
#include "porytiles2/domain/packing/models/prefilled_palette.hpp"
#include "porytiles2/domain/packing/services/backtracking_strategy.hpp"
#include "porytiles2/domain/packing/services/packing_strategy.hpp"

using namespace porytiles2;

namespace {

[[nodiscard]] ColorSet make_color_set(const std::vector<std::size_t> &indices)
{
    ColorSet cs;
    for (std::size_t idx : indices) {
        cs.set(ColorIndex{idx});
    }
    return cs;
}

[[nodiscard]] PackableTile make_regular_tile(std::size_t tile_index, const std::vector<std::size_t> &color_indices)
{
    return PackableTile{PackableTile::RegularId{tile_index}, make_color_set(color_indices)};
}

[[nodiscard]] std::bitset<pal::num_pals> all_palettes_available()
{
    std::bitset<pal::num_pals> bits;
    bits.set();
    return bits;
}

[[nodiscard]] std::bitset<pal::num_pals> n_palettes_available(std::size_t n)
{
    std::bitset<pal::num_pals> bits;
    for (std::size_t i = 0; i < n && i < pal::num_pals; ++i) {
        bits.set(i);
    }
    return bits;
}

[[nodiscard]] PackingInput
make_input(std::vector<PackableTile> tiles, std::bitset<pal::num_pals> available_pals, std::size_t capacity = 15)
{
    return PackingInput{
        .tiles_ = std::move(tiles),
        .hints_ = {},
        .prefilled_pals_ = {},
        .pal_pool_ = PalettePool{available_pals},
        .pal_capacity_ = capacity,
    };
}

} // namespace

// =============================================================================
// Test: BasicSingleTile
// =============================================================================

TEST(BacktrackingStrategyTest, BasicSingleTile)
{
    auto tile = make_regular_tile(0, {1, 2, 3});
    auto input = make_input({tile}, all_palettes_available());

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    ASSERT_EQ(output.tile_to_pal_.size(), 1u);
    ASSERT_TRUE(output.tile_to_pal_.contains(tile.id()));
}

// =============================================================================
// Test: OverlappingTilesSharePalette
// =============================================================================

TEST(BacktrackingStrategyTest, OverlappingTilesSharePalette)
{
    auto tile_a = make_regular_tile(0, {1, 2, 3});
    auto tile_b = make_regular_tile(1, {2, 3, 4});
    auto input = make_input({tile_a, tile_b}, all_palettes_available());

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    ASSERT_EQ(output.tile_to_pal_.size(), 2u);

    // Both tiles should share a palette since they overlap well
    std::size_t pal_a = output.tile_to_pal_.at(tile_a.id());
    std::size_t pal_b = output.tile_to_pal_.at(tile_b.id());
    EXPECT_EQ(pal_a, pal_b);
}

// =============================================================================
// Test: DisjointTilesUseSeparatePalettes
// =============================================================================

TEST(BacktrackingStrategyTest, DisjointTilesUseSeparatePalettes)
{
    auto tile_a = make_regular_tile(0, {1, 2, 3});
    auto tile_b = make_regular_tile(1, {4, 5, 6});
    // Capacity=3 forces them into separate palettes
    auto input = make_input({tile_a, tile_b}, all_palettes_available(), 3);

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    ASSERT_EQ(output.tile_to_pal_.size(), 2u);
    EXPECT_NE(output.tile_to_pal_.at(tile_a.id()), output.tile_to_pal_.at(tile_b.id()));
}

// =============================================================================
// Test: TightPackingRequiresBacktracking
// =============================================================================

TEST(BacktrackingStrategyTest, TightPackingRequiresBacktracking)
{
    /*
     * Construct a scenario where greedy FFD ordering would fail but backtracking succeeds.
     * 4 tiles, 2 palettes, capacity=6:
     * - tile_a: {1,2,3,4}
     * - tile_b: {3,4,5,6}
     * - tile_c: {5,6,7,8}
     * - tile_d: {7,8,1,2}
     *
     * Valid solution: pal0={tile_a, tile_b} => {1,2,3,4,5,6}=6, pal1={tile_c, tile_d} => {5,6,7,8,1,2}=6
     * Greedy FFD might place tile_a and tile_c together (no overlap) causing {1,2,3,4,5,6,7,8}=8 > 6.
     */
    auto tile_a = make_regular_tile(0, {1, 2, 3, 4});
    auto tile_b = make_regular_tile(1, {3, 4, 5, 6});
    auto tile_c = make_regular_tile(2, {5, 6, 7, 8});
    auto tile_d = make_regular_tile(3, {7, 8, 1, 2});

    auto input = make_input({tile_a, tile_b, tile_c, tile_d}, n_palettes_available(2), 6);

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value()) << "Backtracking should find a valid packing for tight instances";
    auto &output = result.value();

    ASSERT_EQ(output.tile_to_pal_.size(), 4u);
    for (const auto &tile : {tile_a, tile_b, tile_c, tile_d}) {
        ASSERT_TRUE(output.tile_to_pal_.contains(tile.id()));
    }
}

// =============================================================================
// Test: PrefilledPalettesRespected
// =============================================================================

TEST(BacktrackingStrategyTest, PrefilledPalettesRespected)
{
    ColorSet prefilled_colors = make_color_set({1, 2, 3});
    auto prefilled = PrefilledPalette::partially_locked(0, prefilled_colors, 3, 15);

    // Tile overlaps with prefilled palette
    auto tile = make_regular_tile(0, {1, 2, 4, 5});

    PackingInput input{
        .tiles_ = {tile},
        .hints_ = {},
        .prefilled_pals_ = {prefilled},
        .pal_pool_ = PalettePool{all_palettes_available()},
        .pal_capacity_ = 15,
    };

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    ASSERT_TRUE(output.tile_to_pal_.contains(tile.id()));

    // Verify prefilled palette system tile is preserved
    bool found_prefilled = false;
    for (const auto &pal : output.pals_) {
        if (pal.hardware_index() == 0) {
            for (const auto &tid : pal.assigned_tile_ids()) {
                if (std::holds_alternative<PackableTile::PrefilledPaletteId>(tid)) {
                    found_prefilled = true;
                }
            }
        }
    }
    EXPECT_TRUE(found_prefilled) << "Prefilled palette system tile should be preserved";
}

// =============================================================================
// Test: HintTilesProcessed
// =============================================================================

TEST(BacktrackingStrategyTest, HintTilesProcessed)
{
    auto hint = PackableTile{PackableTile::HintId{"test_hint"}, make_color_set({1, 2, 3})};
    auto tile = make_regular_tile(0, {1, 2, 4});

    PackingInput input{
        .tiles_ = {tile},
        .hints_ = {hint},
        .prefilled_pals_ = {},
        .pal_pool_ = PalettePool{all_palettes_available()},
        .pal_capacity_ = 15,
    };

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    // Both hint and regular tile should be assigned
    ASSERT_TRUE(output.tile_to_pal_.contains(hint.id()));
    ASSERT_TRUE(output.tile_to_pal_.contains(tile.id()));

    // They share colors, so they should be in the same palette
    EXPECT_EQ(output.tile_to_pal_.at(hint.id()), output.tile_to_pal_.at(tile.id()));
}

// =============================================================================
// Test: EmptyInput
// =============================================================================

TEST(BacktrackingStrategyTest, EmptyInput)
{
    auto input = make_input({}, all_palettes_available());

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    EXPECT_TRUE(output.tile_to_pal_.empty());
}

// =============================================================================
// Test: ImpossibleInput
// =============================================================================

TEST(BacktrackingStrategyTest, ImpossibleInput)
{
    // Tile needs 5 colors but capacity is only 4
    auto tile = make_regular_tile(0, {1, 2, 3, 4, 5});
    auto input = make_input({tile}, n_palettes_available(1), 4);

    BacktrackingStrategy strategy{};
    auto result = strategy.pack(input);

    EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Test: Deterministic
// =============================================================================

TEST(BacktrackingStrategyTest, Deterministic)
{
    auto tile_a = make_regular_tile(0, {1, 2, 3, 4});
    auto tile_b = make_regular_tile(1, {3, 4, 5, 6});
    auto tile_c = make_regular_tile(2, {5, 6, 7, 8});

    auto input1 = make_input({tile_a, tile_b, tile_c}, all_palettes_available());
    auto input2 = make_input({tile_a, tile_b, tile_c}, all_palettes_available());

    BacktrackingStrategy strategy1{};
    BacktrackingStrategy strategy2{};

    auto result1 = strategy1.pack(input1);
    auto result2 = strategy2.pack(input2);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());

    EXPECT_EQ(result1.value().tile_to_pal_, result2.value().tile_to_pal_);
}

// =============================================================================
// Test: SingleConfigDfsSucceeds
// =============================================================================

TEST(BacktrackingStrategyTest, SingleConfigDfsSucceeds)
{
    auto tile_a = make_regular_tile(0, {1, 2, 3});
    auto tile_b = make_regular_tile(1, {2, 3, 4});
    auto input = make_input({tile_a, tile_b}, all_palettes_available());

    BacktrackingStrategy strategy{SearchAlgorithm::dfs, 1'000'000, std::numeric_limits<std::size_t>::max(), true};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    ASSERT_EQ(output.tile_to_pal_.size(), 2u);
    EXPECT_EQ(output.tile_to_pal_.at(tile_a.id()), output.tile_to_pal_.at(tile_b.id()));
}

// =============================================================================
// Test: SingleConfigBfsSucceeds
// =============================================================================

TEST(BacktrackingStrategyTest, SingleConfigBfsSucceeds)
{
    auto tile_a = make_regular_tile(0, {1, 2, 3});
    auto tile_b = make_regular_tile(1, {2, 3, 4});
    auto input = make_input({tile_a, tile_b}, all_palettes_available());

    BacktrackingStrategy strategy{SearchAlgorithm::bfs, 1'000'000, std::numeric_limits<std::size_t>::max(), true};
    auto result = strategy.pack(input);

    ASSERT_TRUE(result.has_value());
    auto &output = result.value();

    ASSERT_EQ(output.tile_to_pal_.size(), 2u);
    EXPECT_EQ(output.tile_to_pal_.at(tile_a.id()), output.tile_to_pal_.at(tile_b.id()));
}

// =============================================================================
// Test: SingleConfigCutoffTooLow
// =============================================================================

TEST(BacktrackingStrategyTest, SingleConfigCutoffTooLow)
{
    /*
     * Use the tight packing scenario that requires real backtracking, but with
     * a very low node cutoff so the single config fails (no preset matrix fallback).
     */
    auto tile_a = make_regular_tile(0, {1, 2, 3, 4});
    auto tile_b = make_regular_tile(1, {3, 4, 5, 6});
    auto tile_c = make_regular_tile(2, {5, 6, 7, 8});
    auto tile_d = make_regular_tile(3, {7, 8, 1, 2});

    auto input = make_input({tile_a, tile_b, tile_c, tile_d}, n_palettes_available(2), 6);

    BacktrackingStrategy strategy{SearchAlgorithm::dfs, 1, 1, false};
    auto result = strategy.pack(input);

    EXPECT_FALSE(result.has_value()) << "Single config with cutoff=1 should fail without preset matrix fallback";
}

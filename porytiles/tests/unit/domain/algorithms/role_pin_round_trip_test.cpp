#include "porytiles/domain/algorithms/role_pin_round_trip.hpp"

#include <optional>

#include "gtest/gtest.h"

#include "porytiles/domain/models/layer.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"

namespace porytiles {
namespace {

// Builds an unpinned attribute whose layer_type reads back as the bin-decoded value, as unpack_metatile_attribute
// produces it (plain setter, no explicit pin).
MetatileAttribute bin_attribute(LayerType layer_type)
{
    MetatileAttribute attribute{};
    attribute.layer_type(layer_type);
    return attribute;
}

// Builds a prior attribute carrying an explicit pin, as the CSV loader produces for a filled pin cell.
MetatileAttribute pinned_attribute(LayerType layer_type)
{
    MetatileAttribute attribute{};
    attribute.explicit_layer_type(layer_type);
    return attribute;
}

// Rule 1: a fresh import / decompile with no CSV pins every row from the bin.
TEST(RolePinRoundTripTest, NoCsvPinsEveryRowFromBin)
{
    const MetatileAttribute merged =
        merge_prior_layer_type_pin(bin_attribute(LayerType::covered), PriorPinColumnState::no_csv, std::nullopt);
    ASSERT_TRUE(merged.explicit_layer_type().has_value());
    EXPECT_EQ(merged.explicit_layer_type().value(), LayerType::covered);
}

// Rule 2a: a decompile whose CSV lacked the active pin column adds it and pins every row from the bin.
TEST(RolePinRoundTripTest, ColumnAbsentPinsEveryRowFromBin)
{
    const MetatileAttribute merged = merge_prior_layer_type_pin(
        bin_attribute(LayerType::split), PriorPinColumnState::column_absent, bin_attribute(LayerType::normal));
    ASSERT_TRUE(merged.explicit_layer_type().has_value());
    EXPECT_EQ(merged.explicit_layer_type().value(), LayerType::split);
}

// Rule 2b-i: with the column present, a prior blank (unpinned) cell stays unpinned so inference is trusted.
TEST(RolePinRoundTripTest, ColumnPresentBlankPriorStaysUnpinned)
{
    const MetatileAttribute merged = merge_prior_layer_type_pin(
        bin_attribute(LayerType::covered), PriorPinColumnState::column_present, bin_attribute(LayerType::covered));
    EXPECT_FALSE(merged.explicit_layer_type().has_value());
}

// Rule 2b-ii: with the column present, a prior filled (pinned) cell stays pinned, but the value is refreshed from the
// bin rather than carried over from the old CSV.
TEST(RolePinRoundTripTest, ColumnPresentFilledPriorStaysPinnedWithBinValue)
{
    const MetatileAttribute merged = merge_prior_layer_type_pin(
        bin_attribute(LayerType::covered), PriorPinColumnState::column_present, pinned_attribute(LayerType::split));
    ASSERT_TRUE(merged.explicit_layer_type().has_value());
    // The prior pin was 'split', but the value comes from the bin ('covered').
    EXPECT_EQ(merged.explicit_layer_type().value(), LayerType::covered);
}

// A row absent from the prior CSV (metatile count grew, or the file was hand-trimmed) stays unpinned, like a blank
// cell.
TEST(RolePinRoundTripTest, ColumnPresentAbsentPriorRowStaysUnpinned)
{
    const MetatileAttribute merged =
        merge_prior_layer_type_pin(bin_attribute(LayerType::split), PriorPinColumnState::column_present, std::nullopt);
    EXPECT_FALSE(merged.explicit_layer_type().has_value());
}

} // namespace
} // namespace porytiles

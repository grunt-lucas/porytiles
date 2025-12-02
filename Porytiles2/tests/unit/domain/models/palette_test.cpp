#include "porytiles2/domain/models/palette.hpp"

#include <array>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

// =============================================================================
// Dynamic Palette (N=0) Tests
// =============================================================================

TEST(PaletteDynamicTests, DefaultConstructorCreatesEmptyPalette)
{
    Palette<Rgba32> pal{};
    EXPECT_EQ(pal.size(), 0);
}

TEST(PaletteDynamicTests, VectorConstructorCreatesPaletteWithColors)
{
    std::vector<Rgba32> colors{Rgba32{255, 0, 0, 255}, Rgba32{0, 255, 0, 255}, Rgba32{0, 0, 255, 255}};
    Palette<Rgba32> pal{colors};

    EXPECT_EQ(pal.size(), 3);
    EXPECT_EQ(pal.at(0), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(pal.at(1), Rgba32(0, 255, 0, 255));
    EXPECT_EQ(pal.at(2), Rgba32(0, 0, 255, 255));
}

TEST(PaletteDynamicTests, FillConstructorCreates16Copies)
{
    Rgba32 color{128, 128, 128, 255};
    Palette<Rgba32> pal{color};

    EXPECT_EQ(pal.size(), 16);
    for (std::size_t i = 0; i < 16; i++) {
        EXPECT_EQ(pal.at(i), color);
    }
}

TEST(PaletteDynamicTests, AddAppendsColor)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 0, 255});
    pal.add(Rgba32{0, 255, 0, 255});

    EXPECT_EQ(pal.size(), 2);
    EXPECT_EQ(pal.at(0), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(pal.at(1), Rgba32(0, 255, 0, 255));
}

TEST(PaletteDynamicTests, AddWildcardAppendsWildcardSlot)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 0, 255});
    pal.add_wildcard();
    pal.add(Rgba32{0, 0, 255, 255});

    EXPECT_EQ(pal.size(), 3);
    EXPECT_FALSE(pal.is_wildcard(0));
    EXPECT_TRUE(pal.is_wildcard(1));
    EXPECT_FALSE(pal.is_wildcard(2));
}

TEST(PaletteDynamicTests, SizeReturnsVectorSize)
{
    Palette<Rgba32> pal{};
    EXPECT_EQ(pal.size(), 0);

    pal.add(Rgba32{0, 0, 0, 255});
    EXPECT_EQ(pal.size(), 1);

    pal.add(Rgba32{0, 0, 0, 255});
    pal.add(Rgba32{0, 0, 0, 255});
    EXPECT_EQ(pal.size(), 3);
}

// =============================================================================
// Fixed-Size Palette (N>0) Tests
// =============================================================================

TEST(PaletteFixedTests, DefaultConstructorCreatesAllWildcards)
{
    Palette<Rgba32, 16> pal{};

    EXPECT_EQ(pal.size(), 16);
    for (std::size_t i = 0; i < 16; i++) {
        EXPECT_TRUE(pal.is_wildcard(i));
    }
}

TEST(PaletteFixedTests, ArrayConstructorCreatesPaletteWithColors)
{
    std::array<Rgba32, 4> colors{
        Rgba32{255, 0, 0, 255}, Rgba32{0, 255, 0, 255}, Rgba32{0, 0, 255, 255}, Rgba32{255, 255, 0, 255}};
    Palette<Rgba32, 4> pal{colors};

    EXPECT_EQ(pal.size(), 4);
    EXPECT_EQ(pal.at(0), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(pal.at(1), Rgba32(0, 255, 0, 255));
    EXPECT_EQ(pal.at(2), Rgba32(0, 0, 255, 255));
    EXPECT_EQ(pal.at(3), Rgba32(255, 255, 0, 255));
}

TEST(PaletteFixedTests, FillConstructorFillsAllSlots)
{
    Rgba32 color{64, 64, 64, 255};
    Palette<Rgba32, 8> pal{color};

    EXPECT_EQ(pal.size(), 8);
    for (std::size_t i = 0; i < 8; i++) {
        EXPECT_EQ(pal.at(i), color);
        EXPECT_FALSE(pal.is_wildcard(i));
    }
}

TEST(PaletteFixedTests, SizeReturnsN)
{
    Palette<Rgba32, 4> pal4{};
    Palette<Rgba32, 16> pal16{};
    Palette<Rgba32, 32> pal32{};

    EXPECT_EQ(pal4.size(), 4);
    EXPECT_EQ(pal16.size(), 16);
    EXPECT_EQ(pal32.size(), 32);
}

// =============================================================================
// Common Functionality Tests (Both Variants)
// =============================================================================

TEST(PaletteCommonTests, SetReplacesColorAtIndex)
{
    Palette<Rgba32> dynamic_pal{};
    dynamic_pal.add(Rgba32{0, 0, 0, 255});
    dynamic_pal.add(Rgba32{0, 0, 0, 255});
    dynamic_pal.set(Rgba32{255, 0, 0, 255}, 1);
    EXPECT_EQ(dynamic_pal.at(1), Rgba32(255, 0, 0, 255));

    Palette<Rgba32, 4> fixed_pal{Rgba32{0, 0, 0, 255}};
    fixed_pal.set(Rgba32{0, 255, 0, 255}, 2);
    EXPECT_EQ(fixed_pal.at(2), Rgba32(0, 255, 0, 255));
}

TEST(PaletteCommonTests, SetWildcardMarksSlotAsWildcard)
{
    Palette<Rgba32> dynamic_pal{};
    dynamic_pal.add(Rgba32{255, 0, 0, 255});
    dynamic_pal.add(Rgba32{0, 255, 0, 255});
    EXPECT_FALSE(dynamic_pal.is_wildcard(1));
    dynamic_pal.set_wildcard(1);
    EXPECT_TRUE(dynamic_pal.is_wildcard(1));

    Palette<Rgba32, 4> fixed_pal{Rgba32{0, 0, 0, 255}};
    EXPECT_FALSE(fixed_pal.is_wildcard(2));
    fixed_pal.set_wildcard(2);
    EXPECT_TRUE(fixed_pal.is_wildcard(2));
}

TEST(PaletteCommonTests, AtReturnsColorAtIndex)
{
    Palette<Rgba32> dynamic_pal{};
    dynamic_pal.add(Rgba32{10, 20, 30, 255});
    dynamic_pal.add(Rgba32{40, 50, 60, 255});
    EXPECT_EQ(dynamic_pal.at(0), Rgba32(10, 20, 30, 255));
    EXPECT_EQ(dynamic_pal.at(1), Rgba32(40, 50, 60, 255));

    std::array<Rgba32, 2> colors{Rgba32{70, 80, 90, 255}, Rgba32{100, 110, 120, 255}};
    Palette<Rgba32, 2> fixed_pal{colors};
    EXPECT_EQ(fixed_pal.at(0), Rgba32(70, 80, 90, 255));
    EXPECT_EQ(fixed_pal.at(1), Rgba32(100, 110, 120, 255));
}

TEST(PaletteCommonTests, AtOptionalReturnsOptionalColor)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{255, 0, 0, 255});
    pal.add_wildcard();

    auto opt0 = pal.at_optional(0);
    auto opt1 = pal.at_optional(1);

    EXPECT_TRUE(opt0.has_value());
    EXPECT_EQ(opt0.value(), Rgba32(255, 0, 0, 255));
    EXPECT_FALSE(opt1.has_value());
}

TEST(PaletteCommonTests, SlotZeroColorReturnsSlot0)
{
    Palette<Rgba32> dynamic_pal{};
    dynamic_pal.add(Rgba32{123, 45, 67, 255});
    EXPECT_EQ(dynamic_pal.slot_zero_color(), Rgba32(123, 45, 67, 255));

    std::array<Rgba32, 4> colors{
        Rgba32{89, 10, 11, 255}, Rgba32{0, 0, 0, 255}, Rgba32{0, 0, 0, 255}, Rgba32{0, 0, 0, 255}};
    Palette<Rgba32, 4> fixed_pal{colors};
    EXPECT_EQ(fixed_pal.slot_zero_color(), Rgba32(89, 10, 11, 255));
}

// =============================================================================
// Wildcard Query Tests
// =============================================================================

TEST(PaletteWildcardTests, IsWildcardReturnsTrueForWildcardSlots)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});
    pal.add_wildcard();
    pal.add(Rgba32{0, 0, 0, 255});

    EXPECT_FALSE(pal.is_wildcard(0));
    EXPECT_TRUE(pal.is_wildcard(1));
    EXPECT_FALSE(pal.is_wildcard(2));
}

TEST(PaletteWildcardTests, HasAnyWildcardsReturnsTrueIfAnyWildcardsExist)
{
    Palette<Rgba32> pal_no_wildcards{};
    pal_no_wildcards.add(Rgba32{0, 0, 0, 255});
    pal_no_wildcards.add(Rgba32{0, 0, 0, 255});
    EXPECT_FALSE(pal_no_wildcards.has_any_wildcards());

    Palette<Rgba32> pal_with_wildcard{};
    pal_with_wildcard.add(Rgba32{0, 0, 0, 255});
    pal_with_wildcard.add_wildcard();
    EXPECT_TRUE(pal_with_wildcard.has_any_wildcards());
}

TEST(PaletteWildcardTests, HasAnyWildcardsOnFixedDefaultConstruction)
{
    Palette<Rgba32, 4> pal{};
    EXPECT_TRUE(pal.has_any_wildcards());
}

TEST(PaletteWildcardTests, HasAnyWildcardsOnFixedFilledPalette)
{
    Palette<Rgba32, 4> pal{Rgba32{0, 0, 0, 255}};
    EXPECT_FALSE(pal.has_any_wildcards());
}

TEST(PaletteWildcardTests, EmptyDynamicPaletteHasNoWildcards)
{
    Palette<Rgba32> pal{};
    EXPECT_FALSE(pal.has_any_wildcards());
}

// =============================================================================
// Map Generation Tests
// =============================================================================

TEST(PaletteMapTests, ColorToIndexMapSkipsSlot0AndWildcards)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});   // slot 0 - skipped
    pal.add(Rgba32{255, 0, 0, 255}); // slot 1 - included
    pal.add_wildcard();              // slot 2 - skipped (wildcard)
    pal.add(Rgba32{0, 255, 0, 255}); // slot 3 - included

    auto map = pal.color_to_index_map();

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.at(Rgba32{255, 0, 0, 255}).value(), 1);
    EXPECT_EQ(map.at(Rgba32{0, 255, 0, 255}).value(), 3);
    EXPECT_EQ(map.count(Rgba32{0, 0, 0, 255}), 0); // slot 0 excluded
}

TEST(PaletteMapTests, IndexToColorMapSkipsSlot0AndWildcards)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});   // slot 0 - skipped
    pal.add(Rgba32{255, 0, 0, 255}); // slot 1 - included
    pal.add_wildcard();              // slot 2 - skipped (wildcard)
    pal.add(Rgba32{0, 255, 0, 255}); // slot 3 - included

    auto map = pal.index_to_color_map();

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.at(PaletteIndex{1}), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(map.at(PaletteIndex{3}), Rgba32(0, 255, 0, 255));
    EXPECT_EQ(map.count(PaletteIndex{0}), 0); // slot 0 excluded
    EXPECT_EQ(map.count(PaletteIndex{2}), 0); // wildcard excluded
}

TEST(PaletteMapTests, MapsOnFixedPaletteWithWildcards)
{
    Palette<Rgba32, 4> pal{};
    pal.set(Rgba32{0, 0, 0, 255}, 0);       // slot 0 - skipped
    pal.set(Rgba32{100, 100, 100, 255}, 1); // slot 1 - included
    // slot 2 remains wildcard - skipped
    pal.set(Rgba32{200, 200, 200, 255}, 3); // slot 3 - included

    auto color_map = pal.color_to_index_map();
    auto index_map = pal.index_to_color_map();

    EXPECT_EQ(color_map.size(), 2);
    EXPECT_EQ(index_map.size(), 2);
    EXPECT_EQ(color_map.at(Rgba32{100, 100, 100, 255}).value(), 1);
    EXPECT_EQ(index_map.at(PaletteIndex{3}), Rgba32(200, 200, 200, 255));
}

// =============================================================================
// Death Tests (Panic Conditions)
// =============================================================================

TEST(PaletteDeathTests, AtPanicsOnWildcardSlot)
{
    Palette<Rgba32> pal{};
    pal.add_wildcard();

    EXPECT_DEATH(std::ignore = pal.at(0), "wildcard");
}

TEST(PaletteDeathTests, AtPanicsOnOutOfBoundsIndex)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(std::ignore = pal.at(1), ">=");
    EXPECT_DEATH(std::ignore = pal.at(100), ">=");
}

TEST(PaletteDeathTests, AtOptionalPanicsOnOutOfBoundsIndex)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(std::ignore = pal.at_optional(1), ">=");
}

TEST(PaletteDeathTests, SlotZeroColorPanicsIfSlot0IsWildcard)
{
    Palette<Rgba32, 4> pal{}; // all wildcards

    EXPECT_DEATH(std::ignore = pal.slot_zero_color(), "wildcard");
}

TEST(PaletteDeathTests, SlotZeroColorPanicsOnEmptyPalette)
{
    Palette<Rgba32> pal{};

    EXPECT_DEATH(std::ignore = pal.slot_zero_color(), "zero size");
}

TEST(PaletteDeathTests, SetPanicsOnOutOfBoundsIndex)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(pal.set(Rgba32{255, 255, 255, 255}, 1), ">=");
}

TEST(PaletteDeathTests, SetWildcardPanicsOnOutOfBoundsIndex)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(pal.set_wildcard(1), ">=");
}

TEST(PaletteDeathTests, IsWildcardPanicsOnOutOfBoundsIndex)
{
    Palette<Rgba32> pal{};
    pal.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(std::ignore = pal.is_wildcard(1), ">=");
}

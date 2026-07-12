#include "porytiles/domain/models/palette.hpp"

#include <array>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles/domain/models/rgba32.hpp"

using namespace porytiles;

TEST(PaletteDynamicTests, DefaultEmpty)
{
    Palette<Rgba32> palette{};
    EXPECT_EQ(palette.size(), 0);
}

TEST(PaletteDynamicTests, VectorConstructor)
{
    std::vector<Rgba32> colors{Rgba32{255, 0, 0, 255}, Rgba32{0, 255, 0, 255}, Rgba32{0, 0, 255, 255}};
    Palette<Rgba32> palette{colors};

    EXPECT_EQ(palette.size(), 3);
    EXPECT_EQ(palette.at(0), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(palette.at(1), Rgba32(0, 255, 0, 255));
    EXPECT_EQ(palette.at(2), Rgba32(0, 0, 255, 255));
}

TEST(PaletteDynamicTests, FillConstructor)
{
    Rgba32 color{128, 128, 128, 255};
    Palette<Rgba32> palette{color};

    EXPECT_EQ(palette.size(), 16);
    for (std::size_t i = 0; i < 16; i++) {
        EXPECT_EQ(palette.at(i), color);
    }
}

TEST(PaletteDynamicTests, AddAppendsColor)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{255, 0, 0, 255});
    palette.add(Rgba32{0, 255, 0, 255});

    EXPECT_EQ(palette.size(), 2);
    EXPECT_EQ(palette.at(0), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(palette.at(1), Rgba32(0, 255, 0, 255));
}

TEST(PaletteDynamicTests, AddWildcard)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{255, 0, 0, 255});
    palette.add_wildcard();
    palette.add(Rgba32{0, 0, 255, 255});

    EXPECT_EQ(palette.size(), 3);
    EXPECT_FALSE(palette.is_wildcard(0));
    EXPECT_TRUE(palette.is_wildcard(1));
    EXPECT_FALSE(palette.is_wildcard(2));
}

TEST(PaletteDynamicTests, Size)
{
    Palette<Rgba32> palette{};
    EXPECT_EQ(palette.size(), 0);

    palette.add(Rgba32{0, 0, 0, 255});
    EXPECT_EQ(palette.size(), 1);

    palette.add(Rgba32{0, 0, 0, 255});
    palette.add(Rgba32{0, 0, 0, 255});
    EXPECT_EQ(palette.size(), 3);
}

TEST(PaletteFixedTests, DefaultAllWildcards)
{
    Palette<Rgba32, 16> palette{};

    EXPECT_EQ(palette.size(), 16);
    for (std::size_t i = 0; i < 16; i++) {
        EXPECT_TRUE(palette.is_wildcard(i));
    }
}

TEST(PaletteFixedTests, ArrayConstructor)
{
    std::array<Rgba32, 4> colors{
        Rgba32{255, 0, 0, 255}, Rgba32{0, 255, 0, 255}, Rgba32{0, 0, 255, 255}, Rgba32{255, 255, 0, 255}};
    Palette<Rgba32, 4> palette{colors};

    EXPECT_EQ(palette.size(), 4);
    EXPECT_EQ(palette.at(0), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(palette.at(1), Rgba32(0, 255, 0, 255));
    EXPECT_EQ(palette.at(2), Rgba32(0, 0, 255, 255));
    EXPECT_EQ(palette.at(3), Rgba32(255, 255, 0, 255));
}

TEST(PaletteFixedTests, FillConstructor)
{
    Rgba32 color{64, 64, 64, 255};
    Palette<Rgba32, 8> palette{color};

    EXPECT_EQ(palette.size(), 8);
    for (std::size_t i = 0; i < 8; i++) {
        EXPECT_EQ(palette.at(i), color);
        EXPECT_FALSE(palette.is_wildcard(i));
    }
}

TEST(PaletteFixedTests, Size)
{
    Palette<Rgba32, 4> palette4{};
    Palette<Rgba32, 16> palette16{};
    Palette<Rgba32, 32> palette32{};

    EXPECT_EQ(palette4.size(), 4);
    EXPECT_EQ(palette16.size(), 16);
    EXPECT_EQ(palette32.size(), 32);
}

TEST(PaletteCommonTests, Set)
{
    Palette<Rgba32> dynamic_palette{};
    dynamic_palette.add(Rgba32{0, 0, 0, 255});
    dynamic_palette.add(Rgba32{0, 0, 0, 255});
    dynamic_palette.set(1, Rgba32{255, 0, 0, 255});
    EXPECT_EQ(dynamic_palette.at(1), Rgba32(255, 0, 0, 255));

    Palette<Rgba32, 4> fixed_palette{Rgba32{0, 0, 0, 255}};
    fixed_palette.set(2, Rgba32{0, 255, 0, 255});
    EXPECT_EQ(fixed_palette.at(2), Rgba32(0, 255, 0, 255));
}

TEST(PaletteCommonTests, SetWildcard)
{
    Palette<Rgba32> dynamic_palette{};
    dynamic_palette.add(Rgba32{255, 0, 0, 255});
    dynamic_palette.add(Rgba32{0, 255, 0, 255});
    EXPECT_FALSE(dynamic_palette.is_wildcard(1));
    dynamic_palette.set_wildcard(1);
    EXPECT_TRUE(dynamic_palette.is_wildcard(1));

    Palette<Rgba32, 4> fixed_palette{Rgba32{0, 0, 0, 255}};
    EXPECT_FALSE(fixed_palette.is_wildcard(2));
    fixed_palette.set_wildcard(2);
    EXPECT_TRUE(fixed_palette.is_wildcard(2));
}

TEST(PaletteCommonTests, At)
{
    Palette<Rgba32> dynamic_palette{};
    dynamic_palette.add(Rgba32{10, 20, 30, 255});
    dynamic_palette.add(Rgba32{40, 50, 60, 255});
    EXPECT_EQ(dynamic_palette.at(0), Rgba32(10, 20, 30, 255));
    EXPECT_EQ(dynamic_palette.at(1), Rgba32(40, 50, 60, 255));

    std::array<Rgba32, 2> colors{Rgba32{70, 80, 90, 255}, Rgba32{100, 110, 120, 255}};
    Palette<Rgba32, 2> fixed_palette{colors};
    EXPECT_EQ(fixed_palette.at(0), Rgba32(70, 80, 90, 255));
    EXPECT_EQ(fixed_palette.at(1), Rgba32(100, 110, 120, 255));
}

TEST(PaletteCommonTests, AtOptional)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{255, 0, 0, 255});
    palette.add_wildcard();

    auto opt0 = palette.at_optional(0);
    auto opt1 = palette.at_optional(1);

    EXPECT_TRUE(opt0.has_value());
    EXPECT_EQ(opt0.value(), Rgba32(255, 0, 0, 255));
    EXPECT_FALSE(opt1.has_value());
}

TEST(PaletteCommonTests, SlotZeroColor)
{
    Palette<Rgba32> dynamic_palette{};
    dynamic_palette.add(Rgba32{123, 45, 67, 255});
    EXPECT_EQ(dynamic_palette.slot_zero_color(), Rgba32(123, 45, 67, 255));

    std::array<Rgba32, 4> colors{
        Rgba32{89, 10, 11, 255}, Rgba32{0, 0, 0, 255}, Rgba32{0, 0, 0, 255}, Rgba32{0, 0, 0, 255}};
    Palette<Rgba32, 4> fixed_palette{colors};
    EXPECT_EQ(fixed_palette.slot_zero_color(), Rgba32(89, 10, 11, 255));
}

TEST(PaletteWildcardTests, IsWildcard)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});
    palette.add_wildcard();
    palette.add(Rgba32{0, 0, 0, 255});

    EXPECT_FALSE(palette.is_wildcard(0));
    EXPECT_TRUE(palette.is_wildcard(1));
    EXPECT_FALSE(palette.is_wildcard(2));
}

TEST(PaletteWildcardTests, HasAnyWildcards)
{
    Palette<Rgba32> palette_no_wildcards{};
    palette_no_wildcards.add(Rgba32{0, 0, 0, 255});
    palette_no_wildcards.add(Rgba32{0, 0, 0, 255});
    EXPECT_FALSE(palette_no_wildcards.has_any_wildcards());

    Palette<Rgba32> palette_with_wildcard{};
    palette_with_wildcard.add(Rgba32{0, 0, 0, 255});
    palette_with_wildcard.add_wildcard();
    EXPECT_TRUE(palette_with_wildcard.has_any_wildcards());
}

TEST(PaletteWildcardTests, HasAnyWildcardsFixedDefault)
{
    Palette<Rgba32, 4> palette{};
    EXPECT_TRUE(palette.has_any_wildcards());
}

TEST(PaletteWildcardTests, HasAnyWildcardsFixedFilled)
{
    Palette<Rgba32, 4> palette{Rgba32{0, 0, 0, 255}};
    EXPECT_FALSE(palette.has_any_wildcards());
}

TEST(PaletteWildcardTests, EmptyDynamicNoWildcards)
{
    Palette<Rgba32> palette{};
    EXPECT_FALSE(palette.has_any_wildcards());
}

TEST(PaletteMapTests, ColorToIndexMap)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});   // slot 0 - skipped
    palette.add(Rgba32{255, 0, 0, 255}); // slot 1 - included
    palette.add_wildcard();              // slot 2 - skipped (wildcard)
    palette.add(Rgba32{0, 255, 0, 255}); // slot 3 - included

    auto map = palette.color_to_index_map();

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.at(Rgba32{255, 0, 0, 255}).value(), 1);
    EXPECT_EQ(map.at(Rgba32{0, 255, 0, 255}).value(), 3);
    EXPECT_EQ(map.count(Rgba32{0, 0, 0, 255}), 0); // slot 0 excluded
}

TEST(PaletteMapTests, IndexToColorMap)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});   // slot 0 - skipped
    palette.add(Rgba32{255, 0, 0, 255}); // slot 1 - included
    palette.add_wildcard();              // slot 2 - skipped (wildcard)
    palette.add(Rgba32{0, 255, 0, 255}); // slot 3 - included

    auto map = palette.index_to_color_map();

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.at(PaletteIndex{1}), Rgba32(255, 0, 0, 255));
    EXPECT_EQ(map.at(PaletteIndex{3}), Rgba32(0, 255, 0, 255));
    EXPECT_EQ(map.count(PaletteIndex{0}), 0); // slot 0 excluded
    EXPECT_EQ(map.count(PaletteIndex{2}), 0); // wildcard excluded
}

TEST(PaletteMapTests, MapsOnFixedPaletteWithWildcards)
{
    Palette<Rgba32, 4> palette{};
    palette.set(0, Rgba32{0, 0, 0, 255});       // slot 0 - skipped
    palette.set(1, Rgba32{100, 100, 100, 255}); // slot 1 - included
    // slot 2 remains wildcard - skipped
    palette.set(3, Rgba32{200, 200, 200, 255}); // slot 3 - included

    auto color_map = palette.color_to_index_map();
    auto index_map = palette.index_to_color_map();

    EXPECT_EQ(color_map.size(), 2);
    EXPECT_EQ(index_map.size(), 2);
    EXPECT_EQ(color_map.at(Rgba32{100, 100, 100, 255}).value(), 1);
    EXPECT_EQ(index_map.at(PaletteIndex{3}), Rgba32(200, 200, 200, 255));
}

TEST(PaletteDeathTests, AtPanicsWildcard)
{
    Palette<Rgba32> palette{};
    palette.add_wildcard();

    EXPECT_DEATH(std::ignore = palette.at(0), "wildcard");
}

TEST(PaletteDeathTests, AtPanicsOutOfBounds)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(std::ignore = palette.at(1), ">=");
    EXPECT_DEATH(std::ignore = palette.at(100), ">=");
}

TEST(PaletteDeathTests, AtOptionalPanicsOutOfBounds)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(std::ignore = palette.at_optional(1), ">=");
}

TEST(PaletteDeathTests, SlotZeroColorPanicsWildcard)
{
    Palette<Rgba32, 4> palette{}; // all wildcards

    EXPECT_DEATH(std::ignore = palette.slot_zero_color(), "wildcard");
}

TEST(PaletteDeathTests, SlotZeroColorPanicsEmpty)
{
    Palette<Rgba32> palette{};

    EXPECT_DEATH(std::ignore = palette.slot_zero_color(), "zero size");
}

TEST(PaletteDeathTests, SetPanicsOutOfBounds)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(palette.set(1, Rgba32{255, 255, 255, 255}), ">=");
}

TEST(PaletteDeathTests, SetWildcardPanicsOutOfBounds)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(palette.set_wildcard(1), ">=");
}

TEST(PaletteDeathTests, IsWildcardPanicsOutOfBounds)
{
    Palette<Rgba32> palette{};
    palette.add(Rgba32{0, 0, 0, 255});

    EXPECT_DEATH(std::ignore = palette.is_wildcard(1), ">=");
}

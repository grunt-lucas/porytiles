#include "gtest/gtest.h"

#include "porytiles2/domain/models/tilemap_entry.hpp"

using namespace porytiles2;

TEST(TilemapEntryTests, DefaultConstructedValueShouldBeTransparent)
{
    const TilemapEntry default_entry{};
    EXPECT_TRUE(default_entry.is_transparent());
    EXPECT_EQ(default_entry.tile_index(), 0);
    EXPECT_EQ(default_entry.pal_index(), 0);
    EXPECT_FALSE(default_entry.hflip());
    EXPECT_FALSE(default_entry.vflip());
}

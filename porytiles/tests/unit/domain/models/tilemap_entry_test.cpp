#include "gtest/gtest.h"

#include "porytiles/domain/models/tilemap_entry.hpp"

using namespace porytiles;

TEST(TilemapEntryTests, DefaultIsTransparent)
{
    const TilemapEntry default_entry{};
    EXPECT_TRUE(default_entry.is_transparent());
    EXPECT_EQ(default_entry.tile_index(), 0);
    EXPECT_EQ(default_entry.palette_index(), 0);
    EXPECT_FALSE(default_entry.h_flip());
    EXPECT_FALSE(default_entry.v_flip());
}

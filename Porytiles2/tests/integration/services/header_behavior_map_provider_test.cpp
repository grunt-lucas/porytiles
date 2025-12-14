#include "gtest/gtest.h"

#include <filesystem>

#include "porytiles2/infra/services/header_behavior_map_provider.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

namespace {

const std::filesystem::path kTestResourcesDir = "Resources/Tests/integration/services";

class HeaderBehaviorMapProviderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
};

} // namespace

// =============================================================================
// Define Format Tests
// =============================================================================

TEST_F(HeaderBehaviorMapProviderTest, DefineFormatParsesHexValues)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_define.h", &formatter_, &diag_};

    // Test various hex values from the define format file
    auto normal = provider.lookup("MB_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0x00);

    auto tall_grass = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), 0x02);

    auto deep_water = provider.lookup("MB_DEEP_WATER");
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), 0x12);

    auto counter = provider.lookup("MB_COUNTER");
    ASSERT_TRUE(counter.has_value());
    EXPECT_EQ(counter.value(), 0x80);

    auto sky_pillar = provider.lookup("MB_SKY_PILLAR_CLOSED_DOOR");
    ASSERT_TRUE(sky_pillar.has_value());
    EXPECT_EQ(sky_pillar.value(), 0xEA);
}

TEST_F(HeaderBehaviorMapProviderTest, DefineFormatSkipsMbInvalid)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_define.h", &formatter_, &diag_};

    auto invalid = provider.lookup("MB_INVALID");
    EXPECT_FALSE(invalid.has_value());
}

TEST_F(HeaderBehaviorMapProviderTest, DefineFormatHandlesAbridgedFile)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_abridged.h", &formatter_, &diag_};

    auto normal = provider.lookup("MB_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0x00);

    auto tall_grass = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), 0x02);

    // Only 3 defines in abridged file, so this should not exist
    auto deep_water = provider.lookup("MB_DEEP_WATER");
    EXPECT_FALSE(deep_water.has_value());
}

// =============================================================================
// Enum Format Tests
// =============================================================================

TEST_F(HeaderBehaviorMapProviderTest, EnumFormatParsesCounterBasedValues)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_enum.h", &formatter_, &diag_};

    // Enum values are counter-based starting at 0
    auto normal = provider.lookup("MB_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0);

    auto secret_base_wall = provider.lookup("MB_SECRET_BASE_WALL");
    ASSERT_TRUE(secret_base_wall.has_value());
    EXPECT_EQ(secret_base_wall.value(), 1);

    auto tall_grass = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), 2);

    // MB_DEEP_WATER is at index 18 in the enum
    auto deep_water = provider.lookup("MB_DEEP_WATER");
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), 18);
}

TEST_F(HeaderBehaviorMapProviderTest, EnumFormatHandlesCommentsAfterComma)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_enum.h", &formatter_, &diag_};

    // MB_INTERIOR_DEEP_WATER has a trailing comment in the enum file
    // "MB_INTERIOR_DEEP_WATER, // Used by interior maps; functionally the same as MB_DEEP_WATER"
    auto interior_deep_water = provider.lookup("MB_INTERIOR_DEEP_WATER");
    ASSERT_TRUE(interior_deep_water.has_value());
    EXPECT_EQ(interior_deep_water.value(), 17);
}

TEST_F(HeaderBehaviorMapProviderTest, EnumFormatSkipsMbInvalid)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_enum.h", &formatter_, &diag_};

    // MB_INVALID is defined as UCHAR_MAX in the enum file via #define, not in the enum
    // It should not be in the map
    auto invalid = provider.lookup("MB_INVALID");
    EXPECT_FALSE(invalid.has_value());
}

TEST_F(HeaderBehaviorMapProviderTest, EnumFormatParsesHigherIndexValues)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_enum.h", &formatter_, &diag_};

    // Test some higher-index values to verify counter is working correctly
    auto muddy_slope = provider.lookup("MB_MUDDY_SLOPE");
    ASSERT_TRUE(muddy_slope.has_value());
    EXPECT_EQ(muddy_slope.value(), 208); // 0xD0 in hex equivalent position
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(HeaderBehaviorMapProviderTest, NonExistentFileReturnsErrorOnLookup)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "does_not_exist.h", &formatter_, &diag_};

    auto result = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(diag_.error_tag_counts().at("behavior-header-load-failure"), 1);
}

TEST_F(HeaderBehaviorMapProviderTest, UnknownBehaviorReturnsError)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_define.h", &formatter_, &diag_};

    auto result = provider.lookup("MB_DOES_NOT_EXIST");
    EXPECT_FALSE(result.has_value());

    auto result2 = provider.lookup("NOT_A_BEHAVIOR");
    EXPECT_FALSE(result2.has_value());

    auto result3 = provider.lookup("");
    EXPECT_FALSE(result3.has_value());
}

TEST_F(HeaderBehaviorMapProviderTest, EmptyFileReturnsErrorOnLookup)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_empty.h", &formatter_, &diag_};

    auto result = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(diag_.error_tag_counts().at("behavior-header-load-failure"), 1);
}

// =============================================================================
// Reverse Lookup Tests (value -> name)
// =============================================================================

TEST_F(HeaderBehaviorMapProviderTest, ReverseLookupDefineFormat)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_define.h", &formatter_, &diag_};

    // Reverse lookup: value -> name
    auto normal = provider.lookup(static_cast<std::uint16_t>(0x00));
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), "MB_NORMAL");

    auto tall_grass = provider.lookup(static_cast<std::uint16_t>(0x02));
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), "MB_TALL_GRASS");

    auto deep_water = provider.lookup(static_cast<std::uint16_t>(0x12));
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), "MB_DEEP_WATER");

    auto counter = provider.lookup(static_cast<std::uint16_t>(0x80));
    ASSERT_TRUE(counter.has_value());
    EXPECT_EQ(counter.value(), "MB_COUNTER");
}

TEST_F(HeaderBehaviorMapProviderTest, ReverseLookupEnumFormat)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_enum.h", &formatter_, &diag_};

    // Reverse lookup: value -> name
    auto normal = provider.lookup(static_cast<std::uint16_t>(0));
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), "MB_NORMAL");

    auto secret_base_wall = provider.lookup(static_cast<std::uint16_t>(1));
    ASSERT_TRUE(secret_base_wall.has_value());
    EXPECT_EQ(secret_base_wall.value(), "MB_SECRET_BASE_WALL");

    auto deep_water = provider.lookup(static_cast<std::uint16_t>(18));
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), "MB_DEEP_WATER");
}

TEST_F(HeaderBehaviorMapProviderTest, ReverseLookupUnknownValueReturnsError)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_abridged.h", &formatter_, &diag_};

    // Abridged file only has 3 behaviors (0x00, 0x01, 0x02)
    auto result = provider.lookup(static_cast<std::uint16_t>(0xFF));
    EXPECT_FALSE(result.has_value());

    auto result2 = provider.lookup(static_cast<std::uint16_t>(0x50));
    EXPECT_FALSE(result2.has_value());
}

TEST_F(HeaderBehaviorMapProviderTest, ReverseLookupNonExistentFileReturnsError)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "does_not_exist.h", &formatter_, &diag_};

    auto result = provider.lookup(static_cast<std::uint16_t>(0x00));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(diag_.error_tag_counts().at("behavior-header-load-failure"), 1);
}

TEST_F(HeaderBehaviorMapProviderTest, ReverseLookupEmptyFileReturnsError)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_empty.h", &formatter_, &diag_};

    auto result = provider.lookup(static_cast<std::uint16_t>(0x00));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(diag_.error_tag_counts().at("behavior-header-load-failure"), 1);
}

TEST_F(HeaderBehaviorMapProviderTest, BidirectionalLookupIsConsistent)
{
    HeaderBehaviorMapProvider provider{kTestResourcesDir, "metatile_behaviors_define.h", &formatter_, &diag_};

    // Forward lookup: name -> value
    auto value = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(value.has_value());

    // Reverse lookup: value -> name
    auto name = provider.lookup(value.value());
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "MB_TALL_GRASS");
}

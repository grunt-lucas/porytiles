#include "gtest/gtest.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/infra/services/header_enum_map_provider.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles;

namespace {

const std::filesystem::path test_resources_dir = "resources/tests/integration/infra/services/enum_headers";

// A behavior-equivalent spec: the exact prefix, cap, and format the old dedicated behavior provider used,
// so the ported behavior cases below exercise identical semantics through the consolidated provider.
EnumSpec behavior_spec()
{
    return EnumSpec{
        .prefix = "MB_",
        .max_value = std::numeric_limits<std::uint16_t>::max(),
        .skipped = {},
        .format = HeaderFormat::either,
        .field_display_name = "behavior"};
}

// Terrain and encounter specs mirror the stock FireRed layout: sequential enum members, tight caps.
EnumSpec terrain_spec()
{
    return EnumSpec{
        .prefix = "TILE_TERRAIN_",
        .max_value = 0x1F,
        .skipped = {},
        .format = HeaderFormat::enums_only,
        .field_display_name = "terrain"};
}

EnumSpec encounter_spec()
{
    return EnumSpec{
        .prefix = "TILE_ENCOUNTER_",
        .max_value = 0x07,
        .skipped = {},
        .format = HeaderFormat::enums_only,
        .field_display_name = "encounter_type"};
}

class HeaderEnumMapProviderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
};

} // namespace

// The following cases are ported from the old header_behavior_map_provider_test.cpp. They run the stock
// behavior spec through the consolidated provider and must behave identically to the dedicated class.

TEST_F(HeaderEnumMapProviderTest, DefineFormatParsesHexValues)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_define.h", behavior_spec(), &formatter_, &diag_};

    auto normal = provider.lookup("MB_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0x00u);

    auto tall_grass = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), 0x02u);

    auto deep_water = provider.lookup("MB_DEEP_WATER");
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), 0x12u);

    auto counter = provider.lookup("MB_COUNTER");
    ASSERT_TRUE(counter.has_value());
    EXPECT_EQ(counter.value(), 0x80u);

    auto sky_pillar = provider.lookup("MB_SKY_PILLAR_CLOSED_DOOR");
    ASSERT_TRUE(sky_pillar.has_value());
    EXPECT_EQ(sky_pillar.value(), 0xEAu);
}

TEST_F(HeaderEnumMapProviderTest, DefineFormatAllowsMbInvalid)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_define.h", behavior_spec(), &formatter_, &diag_};

    auto invalid = provider.lookup("MB_INVALID");
    ASSERT_TRUE(invalid.has_value());
    EXPECT_EQ(invalid.value(), 255u);
}

TEST_F(HeaderEnumMapProviderTest, DefineFormatHandlesAbridgedFile)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_abridged.h", behavior_spec(), &formatter_, &diag_};

    auto normal = provider.lookup("MB_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0x00u);

    auto tall_grass = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), 0x02u);

    // Only 3 defines in abridged file, so this should not exist
    auto deep_water = provider.lookup("MB_DEEP_WATER");
    EXPECT_FALSE(deep_water.has_value());
}

TEST_F(HeaderEnumMapProviderTest, EnumFormatParsesCounterBasedValues)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_enum.h", behavior_spec(), &formatter_, &diag_};

    auto normal = provider.lookup("MB_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0u);

    auto secret_base_wall = provider.lookup("MB_SECRET_BASE_WALL");
    ASSERT_TRUE(secret_base_wall.has_value());
    EXPECT_EQ(secret_base_wall.value(), 1u);

    auto tall_grass = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), 2u);

    auto deep_water = provider.lookup("MB_DEEP_WATER");
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), 18u);
}

TEST_F(HeaderEnumMapProviderTest, EnumFormatHandlesCommentsAfterComma)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_enum.h", behavior_spec(), &formatter_, &diag_};

    auto interior_deep_water = provider.lookup("MB_INTERIOR_DEEP_WATER");
    ASSERT_TRUE(interior_deep_water.has_value());
    EXPECT_EQ(interior_deep_water.value(), 17u);
}

TEST_F(HeaderEnumMapProviderTest, EnumFormatAllowsMbInvalid)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_enum.h", behavior_spec(), &formatter_, &diag_};

    // MB_INVALID is defined as UCHAR_MAX via #define, not in the enum. With HeaderFormat::either it still loads.
    auto invalid = provider.lookup("MB_INVALID");
    ASSERT_TRUE(invalid.has_value());
    EXPECT_EQ(invalid.value(), 255u);
}

TEST_F(HeaderEnumMapProviderTest, EnumFormatParsesHigherIndexValues)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_enum.h", behavior_spec(), &formatter_, &diag_};

    auto muddy_slope = provider.lookup("MB_MUDDY_SLOPE");
    ASSERT_TRUE(muddy_slope.has_value());
    EXPECT_EQ(muddy_slope.value(), 208u);
}

TEST_F(HeaderEnumMapProviderTest, NonExistentFileReturnsErrorOnLookup)
{
    HeaderEnumMapProvider provider{test_resources_dir / "does_not_exist.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(result.has_value());
}

TEST_F(HeaderEnumMapProviderTest, UnknownBehaviorReturnsError)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_define.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup("MB_DOES_NOT_EXIST");
    EXPECT_FALSE(result.has_value());

    auto result2 = provider.lookup("NOT_A_BEHAVIOR");
    EXPECT_FALSE(result2.has_value());

    auto result3 = provider.lookup("");
    EXPECT_FALSE(result3.has_value());
}

TEST_F(HeaderEnumMapProviderTest, EmptyFileReturnsErrorOnLookup)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_empty.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(result.has_value());
}

TEST_F(HeaderEnumMapProviderTest, ReverseLookupDefineFormat)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_define.h", behavior_spec(), &formatter_, &diag_};

    auto normal = provider.lookup(static_cast<std::uint32_t>(0x00));
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), "MB_NORMAL");

    auto tall_grass = provider.lookup(static_cast<std::uint32_t>(0x02));
    ASSERT_TRUE(tall_grass.has_value());
    EXPECT_EQ(tall_grass.value(), "MB_TALL_GRASS");

    auto deep_water = provider.lookup(static_cast<std::uint32_t>(0x12));
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), "MB_DEEP_WATER");

    auto counter = provider.lookup(static_cast<std::uint32_t>(0x80));
    ASSERT_TRUE(counter.has_value());
    EXPECT_EQ(counter.value(), "MB_COUNTER");
}

TEST_F(HeaderEnumMapProviderTest, ReverseLookupEnumFormat)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_enum.h", behavior_spec(), &formatter_, &diag_};

    auto normal = provider.lookup(static_cast<std::uint32_t>(0));
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), "MB_NORMAL");

    auto secret_base_wall = provider.lookup(static_cast<std::uint32_t>(1));
    ASSERT_TRUE(secret_base_wall.has_value());
    EXPECT_EQ(secret_base_wall.value(), "MB_SECRET_BASE_WALL");

    auto deep_water = provider.lookup(static_cast<std::uint32_t>(18));
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), "MB_DEEP_WATER");
}

TEST_F(HeaderEnumMapProviderTest, ReverseLookupUnknownValueReturnsError)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_abridged.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup(static_cast<std::uint32_t>(0xFF));
    EXPECT_FALSE(result.has_value());

    auto result2 = provider.lookup(static_cast<std::uint32_t>(0x50));
    EXPECT_FALSE(result2.has_value());
}

TEST_F(HeaderEnumMapProviderTest, ReverseLookupNonExistentFileReturnsError)
{
    HeaderEnumMapProvider provider{test_resources_dir / "does_not_exist.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup(static_cast<std::uint32_t>(0x00));
    EXPECT_FALSE(result.has_value());
}

TEST_F(HeaderEnumMapProviderTest, ReverseLookupEmptyFileReturnsError)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_empty.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup(static_cast<std::uint32_t>(0x00));
    EXPECT_FALSE(result.has_value());
}

TEST_F(HeaderEnumMapProviderTest, BidirectionalLookupIsConsistent)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_define.h", behavior_spec(), &formatter_, &diag_};

    auto value = provider.lookup("MB_TALL_GRASS");
    ASSERT_TRUE(value.has_value());

    auto name = provider.lookup(value.value());
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "MB_TALL_GRASS");
}

TEST_F(HeaderEnumMapProviderTest, DuplicateNameReturnsErrorWithSourceLocations)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_duplicate_name.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(result.has_value());

    ASSERT_FALSE(result.chain().empty());
    std::string error_text = result.chain().back()->join(formatter_);

    EXPECT_TRUE(error_text.find("duplicate behavior name") != std::string::npos)
        << "Error should mention 'duplicate behavior name'. Got: " << error_text;
    EXPECT_TRUE(error_text.find("MB_TALL_GRASS") != std::string::npos)
        << "Error should mention the duplicate name 'MB_TALL_GRASS'. Got: " << error_text;
    EXPECT_TRUE(error_text.find("note:") != std::string::npos)
        << "Error should contain 'note:' for original location. Got: " << error_text;
}

TEST_F(HeaderEnumMapProviderTest, DuplicateValueReturnsErrorWithSourceLocations)
{
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_duplicate_value.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(result.has_value());

    ASSERT_FALSE(result.chain().empty());
    std::string error_text = result.chain().back()->join(formatter_);

    EXPECT_TRUE(error_text.find("duplicate behavior value") != std::string::npos)
        << "Error should mention 'duplicate behavior value'. Got: " << error_text;
    EXPECT_TRUE(error_text.find("MB_TALL_GRASS") != std::string::npos)
        << "Error should mention 'MB_TALL_GRASS'. Got: " << error_text;
    EXPECT_TRUE(error_text.find("MB_GRASS_TALL") != std::string::npos)
        << "Error should mention 'MB_GRASS_TALL'. Got: " << error_text;
    EXPECT_TRUE(error_text.find("note:") != std::string::npos)
        << "Error should contain 'note:' for original location. Got: " << error_text;
}

// The cases below are net-new coverage the consolidation makes possible: format dispatch, the skip set,
// the value cap, prefix isolation across a shared header, and field-name-carrying diagnostics.

TEST_F(HeaderEnumMapProviderTest, EnumsOnlyResolvesTerrainSequentiallyAndIgnoresComplexDefine)
{
    // fieldmap_enums.h carries a function-like METATILE_ID(...) define alongside the enums. An enums_only
    // provider never touches parse_defines(), so that complex define cannot derail the load.
    HeaderEnumMapProvider provider{test_resources_dir / "fieldmap_enums.h", terrain_spec(), &formatter_, &diag_};

    auto normal = provider.lookup("TILE_TERRAIN_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0u);

    auto grass = provider.lookup("TILE_TERRAIN_GRASS");
    ASSERT_TRUE(grass.has_value());
    EXPECT_EQ(grass.value(), 1u);

    auto water = provider.lookup("TILE_TERRAIN_WATER");
    ASSERT_TRUE(water.has_value());
    EXPECT_EQ(water.value(), 2u);

    auto waterfall = provider.lookup("TILE_TERRAIN_WATERFALL");
    ASSERT_TRUE(waterfall.has_value());
    EXPECT_EQ(waterfall.value(), 3u);
}

TEST_F(HeaderEnumMapProviderTest, SharedHeaderEncounterResolvesOnlyItsPrefix)
{
    // Second provider over the same file. Terrain and encounter enums both start at 0, but the prefix
    // filter keeps each provider to its own field, so the value 0 maps to only one name here.
    HeaderEnumMapProvider provider{test_resources_dir / "fieldmap_enums.h", encounter_spec(), &formatter_, &diag_};

    auto none = provider.lookup("TILE_ENCOUNTER_NONE");
    ASSERT_TRUE(none.has_value());
    EXPECT_EQ(none.value(), 0u);

    auto land = provider.lookup("TILE_ENCOUNTER_LAND");
    ASSERT_TRUE(land.has_value());
    EXPECT_EQ(land.value(), 1u);

    auto water = provider.lookup("TILE_ENCOUNTER_WATER");
    ASSERT_TRUE(water.has_value());
    EXPECT_EQ(water.value(), 2u);

    // Terrain names share the file but not the prefix, so they must not resolve here.
    auto terrain = provider.lookup("TILE_TERRAIN_NORMAL");
    EXPECT_FALSE(terrain.has_value());

    // The reverse mapping of 0 is the encounter name, not the terrain name.
    auto reverse = provider.lookup(static_cast<std::uint32_t>(0));
    ASSERT_TRUE(reverse.has_value());
    EXPECT_EQ(reverse.value(), "TILE_ENCOUNTER_NONE");
}

TEST_F(HeaderEnumMapProviderTest, DefinesOnlyResolvesDefinesAndIgnoresEnumMembers)
{
    EnumSpec spec = behavior_spec();
    spec.format = HeaderFormat::defines_only;
    HeaderEnumMapProvider provider{test_resources_dir / "metatile_behaviors_enum.h", spec, &formatter_, &diag_};

    // MB_INVALID is the only #define in the enum-format file; the enum members must stay invisible.
    auto invalid = provider.lookup("MB_INVALID");
    ASSERT_TRUE(invalid.has_value());
    EXPECT_EQ(invalid.value(), 255u);

    auto normal = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(normal.has_value());
}

TEST_F(HeaderEnumMapProviderTest, SkippedSetExcludesNamedEntries)
{
    EnumSpec spec = behavior_spec();
    spec.skipped = {"MB_TALL_GRASS"};
    HeaderEnumMapProvider provider{test_resources_dir / "metatile_behaviors_define.h", spec, &formatter_, &diag_};

    auto tall_grass = provider.lookup("MB_TALL_GRASS");
    EXPECT_FALSE(tall_grass.has_value());

    // Its value must be absent in the reverse direction too.
    auto reverse = provider.lookup(static_cast<std::uint32_t>(0x02));
    EXPECT_FALSE(reverse.has_value());

    // Other entries are unaffected.
    auto normal = provider.lookup("MB_NORMAL");
    ASSERT_TRUE(normal.has_value());
    EXPECT_EQ(normal.value(), 0x00u);

    auto deep_water = provider.lookup("MB_DEEP_WATER");
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), 0x12u);
}

TEST_F(HeaderEnumMapProviderTest, MaxValueFilterDropsOutOfRangeEntries)
{
    EnumSpec spec = behavior_spec();
    spec.max_value = 0x12;
    HeaderEnumMapProvider provider{test_resources_dir / "metatile_behaviors_define.h", spec, &formatter_, &diag_};

    // 0x12 sits at the cap and loads.
    auto deep_water = provider.lookup("MB_DEEP_WATER");
    ASSERT_TRUE(deep_water.has_value());
    EXPECT_EQ(deep_water.value(), 0x12u);

    // 0x80 exceeds the cap and is silently absent both directions.
    auto counter = provider.lookup("MB_COUNTER");
    EXPECT_FALSE(counter.has_value());

    auto reverse = provider.lookup(static_cast<std::uint32_t>(0x80));
    EXPECT_FALSE(reverse.has_value());
}

TEST_F(HeaderEnumMapProviderTest, EnumsOnlyMissingFileReturnsError)
{
    // An enums_only provider skips the define scan and fails in the enum scan when the header is absent.
    HeaderEnumMapProvider provider{test_resources_dir / "does_not_exist.h", terrain_spec(), &formatter_, &diag_};

    auto result = provider.lookup("TILE_TERRAIN_NORMAL");
    EXPECT_FALSE(result.has_value());

    auto reverse = provider.lookup(static_cast<std::uint32_t>(0));
    EXPECT_FALSE(reverse.has_value());
}

TEST_F(HeaderEnumMapProviderTest, DuplicateInEnumFormatReturnsError)
{
    // The duplicate lives among enum members, so the failure is raised from the enum scan loop rather
    // than the define scan loop.
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_duplicate_enum.h", behavior_spec(), &formatter_, &diag_};

    auto result = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(result.has_value());

    ASSERT_FALSE(result.chain().empty());
    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("duplicate behavior name") != std::string::npos)
        << "Error should mention 'duplicate behavior name'. Got: " << error_text;
}

TEST_F(HeaderEnumMapProviderTest, SecondLookupAfterLoadFailureReportsCachedFailure)
{
    // Once a load fails, the provider caches the failure: a later lookup short-circuits instead of
    // re-parsing the file.
    HeaderEnumMapProvider provider{
        test_resources_dir / "metatile_behaviors_duplicate_name.h", behavior_spec(), &formatter_, &diag_};

    auto first = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(first.has_value());

    auto second = provider.lookup("MB_NORMAL");
    EXPECT_FALSE(second.has_value());
    ASSERT_FALSE(second.chain().empty());
    std::string error_text = second.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("previously failed to load") != std::string::npos)
        << "Second lookup should report the cached load failure. Got: " << error_text;
}

TEST_F(HeaderEnumMapProviderTest, DiagnosticsCarryFieldDisplayName)
{
    HeaderEnumMapProvider provider{test_resources_dir / "fieldmap_enums.h", terrain_spec(), &formatter_, &diag_};

    // Prefix-mismatch message names the field.
    auto mismatch = provider.lookup("MB_NORMAL");
    ASSERT_FALSE(mismatch.has_value());
    ASSERT_FALSE(mismatch.chain().empty());
    std::string mismatch_text = mismatch.chain().back()->join(formatter_);
    EXPECT_TRUE(mismatch_text.find("terrain") != std::string::npos)
        << "Prefix-mismatch message should name the field. Got: " << mismatch_text;

    // Not-found message names the field.
    auto not_found = provider.lookup("TILE_TERRAIN_DOES_NOT_EXIST");
    ASSERT_FALSE(not_found.has_value());
    ASSERT_FALSE(not_found.chain().empty());
    std::string not_found_text = not_found.chain().back()->join(formatter_);
    EXPECT_TRUE(not_found_text.find("terrain") != std::string::npos)
        << "Not-found message should name the field. Got: " << not_found_text;
}

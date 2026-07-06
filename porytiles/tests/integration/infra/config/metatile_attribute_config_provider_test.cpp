#include "porytiles/infra/config/metatile_attribute_config_provider.hpp"

#include <algorithm>
#include <filesystem>

#include <gtest/gtest.h>

#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

class MetatileAttrProviderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diagnostics_;

    [[nodiscard]] const MetatileAttrFieldSpec *find(const MetatileAttrFieldSpecs &specs, const std::string &name)
    {
        auto it =
            std::find_if(specs.begin(), specs.end(), [&](const MetatileAttrFieldSpec &s) { return s.name == name; });
        return it == specs.end() ? nullptr : &*it;
    }
};

// --- Testbed acceptance tests: run against the local decomp checkouts when they are present. ---

TEST_F(MetatileAttrProviderTest, PokeemeraldAcceptance)
{
    const std::filesystem::path root = "./pokeemerald";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald testbed not present";
    }

    MetatileAttributeConfigProvider provider{root, &formatter_, &diagnostics_};
    const auto result = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(result.state, ValidationState::valid) << result.error_message;
    ASSERT_EQ(result.value->size(), 1U);
    const auto *behavior = find(result.value.value(), "behavior");
    ASSERT_NE(behavior, nullptr);
    EXPECT_EQ(behavior->mask.value(), 0x00FFU);
    ASSERT_TRUE(behavior->provider.has_value());
    EXPECT_EQ(behavior->provider->prefix, "MB_");
}

TEST_F(MetatileAttrProviderTest, PokefireredAcceptance)
{
    const std::filesystem::path root = "./pokefirered";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokefirered testbed not present";
    }

    MetatileAttributeConfigProvider provider{root, &formatter_, &diagnostics_};
    const auto result = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(result.state, ValidationState::valid) << result.error_message;
    const auto &specs = result.value.value();
    ASSERT_EQ(specs.size(), 7U);

    EXPECT_EQ(find(specs, "behavior")->mask.value(), 0x1FFU);
    EXPECT_EQ(find(specs, "terrain")->mask.value(), 0x3E00U);
    EXPECT_EQ(find(specs, "attribute_2")->mask.value(), 0x3C000U);
    EXPECT_EQ(find(specs, "attribute_3")->mask.value(), 0xFC0000U);
    EXPECT_EQ(find(specs, "encounter_type")->mask.value(), 0x7000000U);
    EXPECT_EQ(find(specs, "attribute_5")->mask.value(), 0x18000000U);
    EXPECT_EQ(find(specs, "attribute_7")->mask.value(), 0x80000000U);

    ASSERT_TRUE(find(specs, "behavior")->provider.has_value());
    ASSERT_TRUE(find(specs, "terrain")->provider.has_value());
    EXPECT_EQ(find(specs, "terrain")->provider->prefix, "TILE_TERRAIN_");
    ASSERT_TRUE(find(specs, "encounter_type")->provider.has_value());
    EXPECT_EQ(find(specs, "encounter_type")->provider->prefix, "TILE_ENCOUNTER_");
}

TEST_F(MetatileAttrProviderTest, PokeemeraldExpansionAcceptance)
{
    const std::filesystem::path root = "./pokeemerald-expansion";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald-expansion testbed not present";
    }

    MetatileAttributeConfigProvider provider{root, &formatter_, &diagnostics_};
    const auto result = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(result.state, ValidationState::valid) << result.error_message;
    const auto &specs = result.value.value();
    ASSERT_EQ(specs.size(), 7U);

    // Behavior carries both the emerald-side primary mask and the FRLG mask.
    const auto *behavior = find(specs, "behavior");
    ASSERT_NE(behavior, nullptr);
    EXPECT_EQ(behavior->mask.value(), 0x00FFU);
    ASSERT_TRUE(behavior->frlg_mask.has_value());
    EXPECT_EQ(behavior->frlg_mask.value(), 0x1FFU);

    // The rest are alternate-only: FRLG mask, no primary. sMetatileAttrMasksEmerald must be ignored.
    for (const char *name : {"terrain", "attribute_2", "attribute_3", "encounter_type", "attribute_5", "attribute_7"}) {
        const auto *field = find(specs, name);
        ASSERT_NE(field, nullptr) << name;
        EXPECT_FALSE(field->mask.has_value()) << name;
        EXPECT_TRUE(field->frlg_mask.has_value()) << name;
    }
    EXPECT_EQ(find(specs, "terrain")->frlg_mask.value(), 0x3E00U);
}

// --- Fixture acceptance tests: trimmed replicas checked in under resources/, always available. ---

const std::filesystem::path fixture_base = "resources/tests/integration/infra/config/metatile_attr_inference";

TEST_F(MetatileAttrProviderTest, FixtureEmerald)
{
    MetatileAttributeConfigProvider provider{fixture_base / "emerald", &formatter_, &diagnostics_};
    const auto result = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(result.state, ValidationState::valid) << result.error_message;
    ASSERT_EQ(result.value->size(), 1U);
    EXPECT_EQ(find(result.value.value(), "behavior")->mask.value(), 0x00FFU);
    // The backslash-continuation cross-header define does not derail the scan.
    EXPECT_TRUE(find(result.value.value(), "behavior")->provider.has_value());
}

TEST_F(MetatileAttrProviderTest, FixtureFirered)
{
    MetatileAttributeConfigProvider provider{fixture_base / "firered", &formatter_, &diagnostics_};
    const auto result = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(result.state, ValidationState::valid) << result.error_message;
    const auto &specs = result.value.value();
    ASSERT_EQ(specs.size(), 7U);
    EXPECT_EQ(find(specs, "behavior")->mask.value(), 0x1FFU);
    EXPECT_EQ(find(specs, "encounter_type")->mask.value(), 0x7000000U);
    EXPECT_EQ(find(specs, "encounter_type")->provider->prefix, "TILE_ENCOUNTER_");
}

TEST_F(MetatileAttrProviderTest, FixtureExpansionIgnoresDecoyTable)
{
    MetatileAttributeConfigProvider provider{fixture_base / "expansion", &formatter_, &diagnostics_};
    const auto result = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(result.state, ValidationState::valid) << result.error_message;
    const auto &specs = result.value.value();
    ASSERT_EQ(specs.size(), 7U);
    // Behavior's primary comes from the bare define; its FRLG mask resolves the _FRLG macro referenced in the table.
    EXPECT_EQ(find(specs, "behavior")->mask.value(), 0x00FFU);
    EXPECT_EQ(find(specs, "behavior")->frlg_mask.value(), 0x1FFU);
    // The decoy sMetatileAttrMasksEmerald must not shadow the real table: terrain keeps its FRLG mask.
    EXPECT_FALSE(find(specs, "terrain")->mask.has_value());
    EXPECT_EQ(find(specs, "terrain")->frlg_mask.value(), 0x3E00U);
}

TEST_F(MetatileAttrProviderTest, MissingFieldmapHeaderIsNotProvided)
{
    MetatileAttributeConfigProvider provider{fixture_base / "does_not_exist", &formatter_, &diagnostics_};
    const auto result = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    EXPECT_EQ(result.state, ValidationState::not_provided);
}

TEST_F(MetatileAttrProviderTest, WarningsRoutedToDiagnosticsAndComputedOnce)
{
    MetatileAttributeConfigProvider provider{fixture_base / "warns", &formatter_, &diagnostics_};

    const auto first = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(first.state, ValidationState::valid) << first.error_message;
    EXPECT_EQ(find(first.value.value(), "behavior")->mask.value(), 0x00FFU); // mask wins over the bad shift
    const std::size_t warnings_after_first = diagnostics_.warnings().size();
    EXPECT_GT(warnings_after_first, 0U);

    // A second query is served from cache, so no additional warnings are emitted.
    const auto second = provider.metatile_attr_fields(ConfigScopeType::tileset, "general");
    ASSERT_EQ(second.state, ValidationState::valid);
    EXPECT_EQ(diagnostics_.warnings().size(), warnings_after_first);
}

} // namespace
} // namespace porytiles

#include "porytiles/infra/config/yaml_file_provider.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"

#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

class YamlFileProviderMetatileAttributeTest : public ::testing::Test {
  protected:
    std::filesystem::path project_root_;
    PlainTextFormatter formatter_;

    void SetUp() override
    {
        // The YamlFileProvider caches parsed files by path in a process-wide cache, so each test needs a distinct path.
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        project_root_ = std::filesystem::temp_directory_path() /
                        (std::string{"porytiles_yaml_test_"} + info->test_suite_name() + "_" + info->name());
        std::filesystem::remove_all(project_root_);
        std::filesystem::create_directories(project_root_ / "porytiles");
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(project_root_, ec);
    }

    void write_config(const std::string &yaml)
    {
        std::ofstream out{project_root_ / "porytiles" / "config.yaml"};
        out << yaml;
    }
};

TEST_F(YamlFileProviderMetatileAttributeTest, FieldsListRoundTripWithHexMasksAndBothFormatSpellings)
{
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
      default: 0
      provider:
        header: include/constants/metatile_behaviors.h
        prefix: MB_
        skipped:
          - MB_INVALID
        format: enums_only
    - name: terrain
      mask: 0x3E00
      provider:
        header: include/global.fieldmap.h
        prefix: TILE_TERRAIN_
        format: enums-only
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.metatile_attribute_fields(ConfigScopeType::tileset, "test");
    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_TRUE(result.value.has_value());
    const auto &specs = result.value.value();
    ASSERT_EQ(specs.size(), 2U);

    EXPECT_EQ(specs[0].name, "behavior");
    ASSERT_TRUE(specs[0].mask.has_value());
    EXPECT_EQ(specs[0].mask.value(), 0x00FFU);
    ASSERT_TRUE(specs[0].default_value.has_value());
    EXPECT_EQ(specs[0].default_value.value(), 0U);
    ASSERT_TRUE(specs[0].provider.has_value());
    EXPECT_EQ(specs[0].provider->prefix, "MB_");
    EXPECT_EQ(specs[0].provider->format, HeaderFormat::enums_only);
    EXPECT_TRUE(specs[0].provider->skipped.contains("MB_INVALID"));

    EXPECT_EQ(specs[1].name, "terrain");
    EXPECT_EQ(specs[1].mask.value(), 0x3E00U);
    // The hyphen spelling of the format enum is accepted too.
    ASSERT_TRUE(specs[1].provider.has_value());
    EXPECT_EQ(specs[1].provider->format, HeaderFormat::enums_only);
}

TEST_F(YamlFileProviderMetatileAttributeTest, OverridesRoundTripIncludingProviderNull)
{
    write_config(R"(
fieldmap:
  metatile_attribute_field_overrides:
    behavior:
      mask: 0x01FF
    terrain:
      provider: null
    layer:
      provider:
        prefix: NEW_
        skipped:
          - X
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.metatile_attribute_field_overrides(ConfigScopeType::tileset, "test");
    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_TRUE(result.value.has_value());
    const auto &overrides = result.value.value();
    ASSERT_EQ(overrides.size(), 3U);

    ASSERT_TRUE(overrides.at("behavior").mask.has_value());
    EXPECT_EQ(overrides.at("behavior").mask.value(), 0x1FFU);

    // `provider: null` maps to a remove flag.
    ASSERT_TRUE(overrides.at("terrain").provider.has_value());
    EXPECT_TRUE(overrides.at("terrain").provider->remove);

    ASSERT_TRUE(overrides.at("layer").provider.has_value());
    EXPECT_FALSE(overrides.at("layer").provider->remove);
    ASSERT_TRUE(overrides.at("layer").provider->prefix.has_value());
    EXPECT_EQ(overrides.at("layer").provider->prefix.value(), "NEW_");
    ASSERT_TRUE(overrides.at("layer").provider->skipped.has_value());
    EXPECT_TRUE(overrides.at("layer").provider->skipped->contains("X"));
}

TEST_F(YamlFileProviderMetatileAttributeTest, MalformedMaskIsInvalidWithMessage)
{
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: not_a_number
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.metatile_attribute_fields(ConfigScopeType::tileset, "test");
    EXPECT_EQ(result.state, ValidationState::invalid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(YamlFileProviderMetatileAttributeTest, UnknownFieldKeyIsInvalid)
{
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      msak: 0x00FF
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.metatile_attribute_fields(ConfigScopeType::tileset, "test");
    EXPECT_EQ(result.state, ValidationState::invalid);
    EXPECT_NE(result.error_message.find("msak"), std::string::npos);
}

TEST_F(YamlFileProviderMetatileAttributeTest, WriteLayerTypeColumnBoolParses)
{
    write_config(R"(
fieldmap:
  write_layer_type_column: true
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.write_layer_type_column(ConfigScopeType::tileset, "test");
    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_TRUE(result.value.has_value());
    EXPECT_TRUE(result.value.value());
}

TEST_F(YamlFileProviderMetatileAttributeTest, KnownFieldmapKeysPassUnknownKeyValidation)
{
    write_config(R"(
fieldmap:
  write_layer_type_column: true
  metatile_attribute_declaration_size: 2
)");

    YamlFileProvider provider{nullptr, project_root_};
    // preload_and_validate returns true on validation failure (e.g. an unknown key). Both keys are known, so it passes.
    EXPECT_FALSE(provider.preload_and_validate(ConfigScopeType::tileset, "test"));
}

TEST_F(YamlFileProviderMetatileAttributeTest, DeclarationSizeParses)
{
    write_config(R"(
fieldmap:
  metatile_attribute_declaration_size: 2
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.metatile_attribute_declaration_size(ConfigScopeType::tileset, "test");
    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_TRUE(result.value.has_value());
    ASSERT_TRUE(result.value.value().has_value());
    EXPECT_EQ(result.value.value().value(), 2U);
}

TEST_F(YamlFileProviderMetatileAttributeTest, DeclarationSizeAbsentIsNotProvided)
{
    write_config(R"(
fieldmap:
  write_layer_type_column: true
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.metatile_attribute_declaration_size(ConfigScopeType::tileset, "test");
    EXPECT_EQ(result.state, ValidationState::not_provided);
}

TEST_F(YamlFileProviderMetatileAttributeTest, DeclarationSizeGarbageIsInvalid)
{
    write_config(R"(
fieldmap:
  metatile_attribute_declaration_size: wide
)");

    YamlFileProvider provider{nullptr, project_root_};
    const auto result = provider.metatile_attribute_declaration_size(ConfigScopeType::tileset, "test");
    EXPECT_EQ(result.state, ValidationState::invalid);
}

TEST_F(YamlFileProviderMetatileAttributeTest, RemovedFrlgKeysFailUnknownKeyValidation)
{
    // use_frlg_alternate_masks and metatile_layer_type_mask_frlg were removed with the one-schema-per-project rework:
    // the FRLG-ness of an expansion build is chosen with metatile_attribute_size, not per-tileset layout selection.
    // A config that still sets the stale keys must fail validation so an upgrading user gets a clear error.
    write_config(R"(
fieldmap:
  use_frlg_alternate_masks: always
  metatile_layer_type_mask_frlg: 0x60000000
)");

    BufferedUserDiagnostics diag;
    YamlFileProvider provider{&diag, project_root_};
    // preload_and_validate returns true on validation failure; the keys are now unknown.
    EXPECT_TRUE(provider.preload_and_validate(ConfigScopeType::tileset, "test"));
}

TEST_F(YamlFileProviderMetatileAttributeTest, RemovedBaseGameKeysFailUnknownKeyValidation)
{
    // base_game was removed in the base-game-decomposition work (issue #285): the layout is now inferred from the
    // target decomp and configured through metatile_attribute_fields. A config that still sets the stale key must
    // fail validation so an upgrading user gets a clear error instead of a silently ignored setting.
    write_config(R"(
fieldmap:
  base_game: pokeemerald
)");

    // Validation only runs when a real diagnostics sink is present: with a null diagnostics the provider skips the
    // unknown-key check entirely, so the negative case needs a live sink to exercise it.
    BufferedUserDiagnostics diag;
    YamlFileProvider provider{&diag, project_root_};
    // preload_and_validate returns true on validation failure; the key is now unknown.
    EXPECT_TRUE(provider.preload_and_validate(ConfigScopeType::tileset, "test"));
}

TEST_F(YamlFileProviderMetatileAttributeTest, ReintroducedMetatileAttributeSizeKeyPassesValidation)
{
    // metatile_attribute_size was removed alongside base_game in issue #285 but reintroduced as an explicit override
    // on top of the mask-layout inference (issue #336), so it must validate as a known key again.
    write_config(R"(
fieldmap:
  metatile_attribute_size: 4
)");

    BufferedUserDiagnostics diag;
    YamlFileProvider provider{&diag, project_root_};
    EXPECT_FALSE(provider.preload_and_validate(ConfigScopeType::tileset, "test"));
}

} // namespace
} // namespace porytiles

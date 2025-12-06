#include "gtest/gtest.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#include "porytiles2/infra/config/yaml_file_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

class YamlFileProviderTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        // Create a unique temporary test directory structure for each test
        // This ensures the static YAML cache doesn't cause issues between tests
        const auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   std::chrono::high_resolution_clock::now().time_since_epoch())
                                   .count();
        test_root_ = std::filesystem::temp_directory_path() / ("porytiles_yaml_test_" + std::to_string(timestamp));
        tileset_dir_ = test_root_ / "data" / "tilesets" / "primary" / "test_tileset";

        std::filesystem::create_directories(tileset_dir_);
        std::filesystem::create_directories(tileset_dir_ / "porytiles");
    }

    void TearDown() override
    {
        // Clean up test directory
        if (std::filesystem::exists(test_root_)) {
            std::filesystem::remove_all(test_root_);
        }
    }

    void create_yaml_file(const std::filesystem::path &path, const std::string &content)
    {
        std::ofstream file{path};
        file << content;
    }

    std::filesystem::path test_root_;
    std::filesystem::path tileset_dir_;
    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
};

TEST_F(YamlFileProviderTest, NameReturnsCorrectProviderName)
{
    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    EXPECT_EQ(provider.name(), "YamlFileProvider");
}

TEST_F(YamlFileProviderTest, NumTilesPrimaryParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  num_tiles_in_primary: 512
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 512);
    EXPECT_TRUE(result.source_info.find("porytiles.yaml") != std::string::npos);
    EXPECT_TRUE(result.source_info.find(":3") != std::string::npos); // Line number check
}

TEST_F(YamlFileProviderTest, NumTilesTotalParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  num_tiles_total: 1024
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_total(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 1024);
    EXPECT_TRUE(result.source_info.find("porytiles.yaml") != std::string::npos);
}

TEST_F(YamlFileProviderTest, NumMetatilesPrimaryParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  num_metatiles_in_primary: 256
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_metatiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 256);
}

TEST_F(YamlFileProviderTest, NumMetatilesTotalParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  num_metatiles_total: 512
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_metatiles_total(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 512);
}

TEST_F(YamlFileProviderTest, NumPalsPrimaryParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  num_pals_in_primary: 6
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_pals_in_primary(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 6);
}

TEST_F(YamlFileProviderTest, NumPalsTotalParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  num_pals_total: 13
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_pals_total(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 13);
}

TEST_F(YamlFileProviderTest, MaxMapDataSizeParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  max_map_data_size: 10240
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.max_map_data_size(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 10240);
}

TEST_F(YamlFileProviderTest, NumTilesPerMetatileParsesValidValue)
{
    const std::string yaml_content = R"(
fieldmap:
  num_tiles_per_metatile: 8
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_per_metatile(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 8);
}

TEST_F(YamlFileProviderTest, ExtrinsicTransparencyParsesValidRgb)
{
    const std::string yaml_content = R"(
tileset:
  extrinsic_transparency: [255, 0, 255]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.extrinsic_transparency(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value().red(), 255);
    EXPECT_EQ(result.value.value().green(), 0);
    EXPECT_EQ(result.value.value().blue(), 255);
    EXPECT_EQ(result.value.value().alpha(), 255); // Default opaque alpha
}

TEST_F(YamlFileProviderTest, ExtrinsicTransparencyParsesValidRgba)
{
    const std::string yaml_content = R"(
tileset:
  extrinsic_transparency: [128, 64, 32, 200]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.extrinsic_transparency(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value().red(), 128);
    EXPECT_EQ(result.value.value().green(), 64);
    EXPECT_EQ(result.value.value().blue(), 32);
    EXPECT_EQ(result.value.value().alpha(), 200);
}

TEST_F(YamlFileProviderTest, ExtrinsicTransparencyRejectsInvalidSequenceLength)
{
    const std::string yaml_content = R"(
tileset:
  extrinsic_transparency: [255, 0]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.extrinsic_transparency(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("must have 3 or 4 elements") != std::string::npos);
}

TEST_F(YamlFileProviderTest, ExtrinsicTransparencyRejectsNonSequence)
{
    const std::string yaml_content = R"(
tileset:
  extrinsic_transparency: 255
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.extrinsic_transparency(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("must be a sequence") != std::string::npos);
}

TEST_F(YamlFileProviderTest, TilesPalModeParsesTrueColor)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    tiles_pal_mode: true-color
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.tiles_pal_mode(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), TilesPalMode::true_color);
}

TEST_F(YamlFileProviderTest, TilesPalModeParsesGreyscale)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    tiles_pal_mode: greyscale
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.tiles_pal_mode(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), TilesPalMode::greyscale);
}

TEST_F(YamlFileProviderTest, TilesPalModeRejectsInvalidValue)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    tiles_pal_mode: invalid-mode
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.tiles_pal_mode(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("invalid value") != std::string::npos);
    EXPECT_TRUE(result.error_message.find("expected 'true-color' or 'greyscale'") != std::string::npos);
}

TEST_F(YamlFileProviderTest, ReturnsNotProvidedWhenFileDoesNotExist)
{
    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    EXPECT_EQ(result.state, ValidationState::not_provided);
}

TEST_F(YamlFileProviderTest, ReturnsNotProvidedWhenKeyDoesNotExist)
{
    const std::string yaml_content = R"(
some_other_config:
  value: 42
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    EXPECT_EQ(result.state, ValidationState::not_provided);
}

TEST_F(YamlFileProviderTest, LocalConfigHasHigherPriorityThanNormalConfig)
{
    const std::string normal_config = R"(
fieldmap:
  num_tiles_in_primary: 512
)";
    const std::string local_config = R"(
fieldmap:
  num_tiles_in_primary: 1024
)";

    create_yaml_file(test_root_ / "porytiles.yaml", normal_config);
    create_yaml_file(test_root_ / "porytiles.local.yaml", local_config);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 1024); // Should use local config value
    EXPECT_TRUE(result.source_info.find("porytiles.local.yaml") != std::string::npos);
}

TEST_F(YamlFileProviderTest, TilesetConfigHasHigherPriorityThanProjectConfig)
{
    const std::string project_config = R"(
fieldmap:
  num_tiles_in_primary: 512
)";
    const std::string tileset_config = R"(
fieldmap:
  num_tiles_in_primary: 768
)";

    create_yaml_file(test_root_ / "porytiles.yaml", project_config);
    create_yaml_file(tileset_dir_ / "porytiles" / "porytiles.yaml", tileset_config);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 768); // Should use tileset config value
    EXPECT_TRUE(result.source_info.find("porytiles.yaml") != std::string::npos);
}

TEST_F(YamlFileProviderTest, TilesetLocalConfigHasHighestPriority)
{
    const std::string project_config = R"(
fieldmap:
  num_tiles_primary: 512
)";
    const std::string project_local_config = R"(
fieldmap:
  num_tiles_primary: 768
)";
    const std::string tileset_config = R"(
fieldmap:
  num_tiles_in_primary: 1024
)";
    const std::string tileset_local_config = R"(
fieldmap:
  num_tiles_in_primary: 2048
)";

    create_yaml_file(test_root_ / "porytiles.yaml", project_config);
    create_yaml_file(test_root_ / "porytiles.local.yaml", project_local_config);
    create_yaml_file(tileset_dir_ / "porytiles" / "porytiles.yaml", tileset_config);
    create_yaml_file(tileset_dir_ / "porytiles" / "porytiles.local.yaml", tileset_local_config);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), 2048); // Should use tileset local config value
    EXPECT_TRUE(result.source_info.find("porytiles.local.yaml") != std::string::npos);
}

TEST_F(YamlFileProviderTest, InvalidValueReturnsError)
{
    const std::string yaml_content = R"(
fieldmap:
  num_tiles_in_primary: not_a_number
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("failed to parse") != std::string::npos);
}

TEST_F(YamlFileProviderTest, LineNumberIsIncludedInSource)
{
    const std::string yaml_content = R"(
# Comment line
fieldmap:
  # Another comment
  num_tiles_in_primary: 512
  num_tiles_total: 1024
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto primary_result = provider.num_tiles_in_primary(ConfigScopeType::tileset, "test_tileset");
    auto total_result = provider.num_tiles_total(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(primary_result.state, ValidationState::valid);
    ASSERT_EQ(total_result.state, ValidationState::valid);

    // Check that line numbers are included and different
    EXPECT_TRUE(primary_result.source_info.find(":") != std::string::npos);
    EXPECT_TRUE(total_result.source_info.find(":") != std::string::npos);
    EXPECT_NE(primary_result.source_info, total_result.source_info);
}

TEST_F(YamlFileProviderTest, PalHintsEnabledParsesTrue)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          enabled: true
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints_enabled(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), true);
}

TEST_F(YamlFileProviderTest, PalHintsEnabledParsesFalse)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          enabled: false
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints_enabled(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    EXPECT_EQ(result.value.value(), false);
}

TEST_F(YamlFileProviderTest, PalHintsParsesValidSingleHint)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
     packing:
      hints:
        hints:
          - name: "foliage"
            colors:
              - [12, 190, 20]
              - [40, 210, 10]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_EQ(result.value.value().size(), 1);

    const auto &hint = result.value.value()[0];
    EXPECT_EQ(hint.name(), "foliage");
    EXPECT_EQ(hint.pal().size(), 2);
    EXPECT_EQ(hint.pal().at(0).red(), 12);
    EXPECT_EQ(hint.pal().at(0).green(), 190);
    EXPECT_EQ(hint.pal().at(0).blue(), 20);
    EXPECT_EQ(hint.pal().at(0).alpha(), 255); // Default opaque alpha
    EXPECT_EQ(hint.pal().at(1).red(), 40);
    EXPECT_EQ(hint.pal().at(1).green(), 210);
    EXPECT_EQ(hint.pal().at(1).blue(), 10);
}

TEST_F(YamlFileProviderTest, PalHintsParsesMultipleHints)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints:
            - name: "foliage"
              colors:
                - [12, 190, 20]
            - name: "water"
              colors:
                - [0, 100, 200]
                - [0, 120, 220]
                - [0, 80, 180]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_EQ(result.value.value().size(), 2);

    EXPECT_EQ(result.value.value()[0].name(), "foliage");
    EXPECT_EQ(result.value.value()[0].pal().size(), 1);

    EXPECT_EQ(result.value.value()[1].name(), "water");
    EXPECT_EQ(result.value.value()[1].pal().size(), 3);
    EXPECT_EQ(result.value.value()[1].pal().at(0).blue(), 200);
}

TEST_F(YamlFileProviderTest, PalHintsRejectsColorsWithAlpha)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints:
            - name: "transparent_hint"
              colors:
                - [128, 64, 32, 200]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("must be [r, g, b]") != std::string::npos);
}

TEST_F(YamlFileProviderTest, PalHintsRejectsNonSequence)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints: "not a sequence"
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("must be a sequence") != std::string::npos);
}

TEST_F(YamlFileProviderTest, PalHintsRejectsHintThatIsNotMap)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints:
            - "not a map"
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("must be a map") != std::string::npos);
}

TEST_F(YamlFileProviderTest, PalHintsRejectsMissingName)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints:
            - colors:
                - [12, 190, 20]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("missing required 'name' field") != std::string::npos);
}

TEST_F(YamlFileProviderTest, PalHintsRejectsMissingColors)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints:
            - name: "foliage"
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("missing required 'colors' field") != std::string::npos);
}

const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints:
            - name: "foliage"
              colors: "not a sequence"
)";
TEST_F(YamlFileProviderTest, PalHintsRejectsColorsNotSequence)
{
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("colors' must be a sequence") != std::string::npos);
}

TEST_F(YamlFileProviderTest, PalHintsRejectsInvalidColorLength)
{
    const std::string yaml_content = R"(
tileset:
  compile:
    pals:
      packing:
        hints:
          hints:
            - name: "foliage"
              colors:
                - [12, 190]
)";
    create_yaml_file(test_root_ / "porytiles.yaml", yaml_content);

    ProjectTilesetArtifactKeyProvider key_provider{test_root_, &formatter_, &diag_};
    YamlFileProvider provider{&diag_, test_root_, key_provider};

    auto result = provider.pal_hints(ConfigScopeType::tileset, "test_tileset");

    ASSERT_EQ(result.state, ValidationState::invalid);
    EXPECT_TRUE(result.error_message.find("must be [r, g, b]") != std::string::npos);
}

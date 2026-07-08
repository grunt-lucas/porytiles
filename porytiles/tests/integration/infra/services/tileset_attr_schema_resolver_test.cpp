#include "porytiles/infra/services/tileset_attr_schema_resolver.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/infra/config/yaml_file_provider.hpp"
#include "porytiles/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

// A field set that resolves to a two-byte primary layout and a four-byte FRLG layout: behavior carries both masks,
// while layer_type is FRLG-only and its mask reaches bit 16.
constexpr auto kFieldsYaml = R"(
fieldmap:
  metatile_attr_fields:
    - name: behavior
      mask: 0x00FF
      frlg_mask: 0x01FF
    - name: layer_type
      frlg_mask: 0x30000
)";

constexpr auto kTilesetName = "gTileset_Test";

class TilesetAttrSchemaResolverTest : public ::testing::Test {
  protected:
    std::filesystem::path project_root_;
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;

    void SetUp() override
    {
        // YamlFileProvider caches parsed files by path process-wide, so each test needs a distinct root.
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        project_root_ = std::filesystem::temp_directory_path() /
                        (std::string{"porytiles_resolver_test_"} + info->test_suite_name() + "_" + info->name());
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

    void write_layouts(const std::string &json)
    {
        std::filesystem::create_directories(project_root_ / "data" / "layouts");
        std::ofstream out{project_root_ / "data" / "layouts" / "layouts.json"};
        out << json;
    }

    void write_metatiles_header(const std::string &content)
    {
        std::filesystem::create_directories(project_root_ / "src" / "data" / "tilesets");
        std::ofstream out{project_root_ / "src" / "data" / "tilesets" / "metatiles.h"};
        out << content;
    }

    [[nodiscard]] ChainableResult<ResolvedTilesetAttrSchema> resolve(const std::string &tileset_name)
    {
        std::vector<std::unique_ptr<ConfigProvider>> providers;
        providers.push_back(std::make_unique<YamlFileProvider>(&formatter_, &diag_, project_root_));
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{&formatter_, std::move(providers)};

        ProjectLayoutMetadataProvider layout_metadata{project_root_, &formatter_, &diag_};
        MetatilesHeaderProvider metatiles_header{project_root_, &formatter_};
        TilesetAttrSchemaResolver resolver{&config, &layout_metadata, &metatiles_header, &formatter_, &diag_};
        return resolver.resolve(tileset_name);
    }

    [[nodiscard]] std::string layout_json(const std::string &primary_tileset, const std::string &layout_version_field)
    {
        return std::string{R"({
  "layouts_table_label": "gMapLayouts",
  "layouts": [
    {
      "id": "LAYOUT_TEST",
      "name": "Test_Layout",
      "width": 20,
      "height": 20,
      "primary_tileset": ")"} +
               primary_tileset + R"(",
      "secondary_tileset": "gTileset_Secondary",
      "border_filepath": "data/layouts/Test/border.bin",
      "blockdata_filepath": "data/layouts/Test/map.bin")" +
               layout_version_field + R"(
    }
  ]
})";
    }

    [[nodiscard]] std::string error_text(const ChainableResult<ResolvedTilesetAttrSchema> &result)
    {
        std::string text;
        for (const auto &err : result.chain()) {
            text += err->join(formatter_);
            text += "\n";
        }
        return text;
    }
};

TEST_F(TilesetAttrSchemaResolverTest, AutomaticFrlgSignalResolvesFrlgWidenedToFourBytes)
{
    write_config(kFieldsYaml);
    write_layouts(layout_json(kTilesetName, R"(,
      "layout_version": "frlg")"));

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::frlg);
    EXPECT_EQ(result.value().attr_bytes, 4U);
}

TEST_F(TilesetAttrSchemaResolverTest, NoLayoutsJsonResolvesPrimary)
{
    write_config(kFieldsYaml);
    // No layouts.json written.

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_EQ(result.value().attr_bytes, 2U);
}

TEST_F(TilesetAttrSchemaResolverTest, AlwaysBeatsSignal)
{
    write_config(std::string{kFieldsYaml} + "  use_frlg_alternate_masks: always\n");
    // No layouts.json, so automatic would resolve primary; always forces frlg.

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::frlg);
    EXPECT_EQ(result.value().attr_bytes, 4U);
}

TEST_F(TilesetAttrSchemaResolverTest, NeverBeatsSignal)
{
    write_config(std::string{kFieldsYaml} + "  use_frlg_alternate_masks: never\n");
    write_layouts(layout_json(kTilesetName, R"(,
      "layout_version": "frlg")"));

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_EQ(result.value().attr_bytes, 2U);
}

TEST_F(TilesetAttrSchemaResolverTest, MixedUsageErrorsMentioningEscapeHatch)
{
    write_config(kFieldsYaml);
    write_layouts(std::string{R"({
  "layouts_table_label": "gMapLayouts",
  "layouts": [
    {
      "id": "LAYOUT_FRLG",
      "name": "Frlg_Layout",
      "width": 20, "height": 20,
      "primary_tileset": "gTileset_Test",
      "secondary_tileset": "gTileset_A",
      "border_filepath": "b.bin", "blockdata_filepath": "m.bin",
      "layout_version": "frlg"
    },
    {
      "id": "LAYOUT_EMERALD",
      "name": "Emerald_Layout",
      "width": 20, "height": 20,
      "primary_tileset": "gTileset_Test",
      "secondary_tileset": "gTileset_B",
      "border_filepath": "b.bin", "blockdata_filepath": "m.bin",
      "layout_version": "emerald"
    }
  ]
})"});

    const auto result = resolve(kTilesetName);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("use_frlg_alternate_masks"), std::string::npos) << error_text(result);
}

TEST_F(TilesetAttrSchemaResolverTest, DetectedU32WidthResolvesFourBytesForPrimaryMasks)
{
    // metatiles.h declares u32 attributes, so even a small-mask primary layout resolves to a 4-byte width.
    write_config(kFieldsYaml);
    write_metatiles_header(
        "const u32 gMetatileAttributes_General[] = "
        "INCBIN_U32(\"data/tilesets/primary/general/metatile_attributes.bin\");\n");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_EQ(result.value().attr_bytes, 4U);
}

TEST_F(TilesetAttrSchemaResolverTest, DetectedNarrowWidthWithWideFrlgMaskIsFatal)
{
    // metatiles.h authoritatively declares u16 (2 bytes), but forcing the FRLG layout selects layer_type's frlg_mask
    // (0x30000, bit 17), which needs 4 bytes. The declared width is engine-fixed and shared across all tilesets, so
    // this is a misconfiguration, not a hidden width: it must be a hard error, not a silent widen to a mismatched u32.
    write_config(std::string{kFieldsYaml} + "  use_frlg_alternate_masks: always\n");
    write_metatiles_header(
        "const u16 gMetatileAttributes_General[] = "
        "INCBIN_U16(\"data/tilesets/primary/general/metatile_attributes.bin\");\n");

    const auto result = resolve(kTilesetName);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("metatiles.h"), std::string::npos) << text;
    EXPECT_NE(text.find("layer_type"), std::string::npos) << text; // names the offending field
}

TEST_F(TilesetAttrSchemaResolverTest, MixedU16U32DeclarationsAreFatal)
{
    write_config(kFieldsYaml);
    write_metatiles_header(
        "const u16 gMetatileAttributes_General[] = "
        "INCBIN_U16(\"data/tilesets/primary/general/metatile_attributes.bin\");\n"
        "const u32 gMetatileAttributes_Petalburg[] = "
        "INCBIN_U32(\"data/tilesets/secondary/petalburg/metatile_attributes.bin\");\n");

    const auto result = resolve(kTilesetName);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("Mixed"), std::string::npos) << error_text(result);
}

TEST_F(TilesetAttrSchemaResolverTest, UndetectableWidthWarnsAndAssumesTwoBytes)
{
    write_config(kFieldsYaml);
    // No metatiles.h written: the width cannot be detected. A real 4-byte project with only low-bit masks would get
    // the wrong layout here, so the resolver must say what it assumed instead of silently landing on 2 bytes.

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 2U);
    EXPECT_TRUE(diag_.warning_tag_counts().contains("metatile-attr-schema"));
}

TEST_F(TilesetAttrSchemaResolverTest, DetectedWidthDoesNotWarn)
{
    write_config(kFieldsYaml);
    write_metatiles_header(
        "const u16 gMetatileAttributes_General[] = "
        "INCBIN_U16(\"data/tilesets/primary/general/metatile_attributes.bin\");\n");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 2U);
    EXPECT_FALSE(diag_.warning_tag_counts().contains("metatile-attr-schema"));
}

TEST_F(TilesetAttrSchemaResolverTest, MalformedLayoutsJsonWarnsAndFallsBackToPrimary)
{
    write_config(kFieldsYaml);
    write_layouts("{ this is not valid json ]");

    const auto result = resolve(kTilesetName);
    // A malformed layouts.json is a soft failure: warn and assume the primary layout rather than blocking resolution.
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_TRUE(diag_.warning_tag_counts().contains("frlg-alternate-masks"));
}

TEST_F(TilesetAttrSchemaResolverTest, NonStringLayoutsTableLabelWarnsAndFallsBackToPrimary)
{
    write_config(kFieldsYaml);
    // Syntactically valid JSON whose layouts_table_label has the wrong type. This must take the same warn-and-fall-back
    // path as unparseable JSON, not escape as an uncaught nlohmann type error.
    write_layouts(R"({ "layouts_table_label": 42, "layouts": [] })");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_TRUE(diag_.warning_tag_counts().contains("frlg-alternate-masks"));
}

TEST_F(TilesetAttrSchemaResolverTest, NonArrayLayoutsFieldWarnsAndFallsBackToPrimary)
{
    write_config(kFieldsYaml);
    write_layouts(R"({ "layouts_table_label": "gMapLayouts", "layouts": { "id": "LAYOUT_TEST" } })");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_TRUE(diag_.warning_tag_counts().contains("frlg-alternate-masks"));
}

TEST_F(TilesetAttrSchemaResolverTest, InvalidLayoutVersionValueIsFatal)
{
    write_config(kFieldsYaml);
    // A well-formed layouts.json but a typo'd layout_version for the queried tileset must not silently mean emerald.
    write_layouts(layout_json(kTilesetName, R"(,
      "layout_version": "firered")"));

    const auto result = resolve(kTilesetName);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("firered"), std::string::npos) << error_text(result);
}

// A minimal single-field config whose behavior mask fits either layout in two bytes, isolating layer-mask behavior.
constexpr auto kSimpleFieldsYaml = R"(
fieldmap:
  metatile_attr_fields:
    - name: behavior
      mask: 0x00FF
      frlg_mask: 0x01FF
)";

TEST_F(TilesetAttrSchemaResolverTest, ExplicitPrimaryLayerMaskOverridesConvention)
{
    write_config(std::string{kSimpleFieldsYaml} + "  metatile_layer_type_mask: 0x0300\n");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x0300U);
}

TEST_F(TilesetAttrSchemaResolverTest, ZeroLayerMaskDisablesLayerType)
{
    write_config(std::string{kSimpleFieldsYaml} + "  metatile_layer_type_mask: 0x0\n");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
}

TEST_F(TilesetAttrSchemaResolverTest, FrlgLayoutSelectsFrlgLayerMask)
{
    // Both layer masks are configured; forcing the FRLG layout must pick the FRLG value and widen the word to 4 bytes.
    write_config(
        std::string{kSimpleFieldsYaml} + "  metatile_layer_type_mask: 0xF000\n" +
        "  metatile_layer_type_mask_frlg: 0x60000000\n" + "  use_frlg_alternate_masks: always\n");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::frlg);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
    EXPECT_EQ(result.value().attr_bytes, 4U);
}

TEST_F(TilesetAttrSchemaResolverTest, PrimaryLayoutSelectsPrimaryLayerMask)
{
    // Same config, but forcing the primary layout must pick the primary value and stay two bytes.
    write_config(
        std::string{kSimpleFieldsYaml} + "  metatile_layer_type_mask: 0xF000\n" +
        "  metatile_layer_type_mask_frlg: 0x60000000\n" + "  use_frlg_alternate_masks: never\n");

    const auto result = resolve(kTilesetName);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().layout, AttrSchemaLayout::primary);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
    EXPECT_EQ(result.value().attr_bytes, 2U);
}

} // namespace
} // namespace porytiles

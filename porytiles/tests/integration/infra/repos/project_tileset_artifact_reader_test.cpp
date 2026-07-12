#include "porytiles/infra/repos/project_tileset_artifact_reader.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/infra/config/yaml_file_provider.hpp"
#include "porytiles/infra/services/anim_code_parser.hpp"
#include "porytiles/infra/services/anim_json_parser.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
#include "porytiles/infra/services/jasc_palette_loader.hpp"
#include "porytiles/infra/services/png_indexed_image_loader.hpp"
#include "porytiles/infra/services/png_rgba_image_loader.hpp"
#include "porytiles/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/infra/services/tileset_attribute_schema_cache.hpp"
#include "porytiles/infra/services/tileset_attribute_schema_resolver.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"
#include "support/mock_infra_config.hpp"

namespace porytiles {
namespace {

// A field set whose two layouts differ observably: the primary layout carries only behavior (mask 0x00FF), while the
// FRLG layout widens behavior to 0x01FF and adds terrain.
constexpr auto kDivergentFieldsYaml = R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
      frlg_mask: 0x01FF
    - name: terrain
      frlg_mask: 0x0E00
)";

class ProjectTilesetArtifactReaderTest : public ::testing::Test {
  protected:
    std::filesystem::path project_root_;
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;
    MockInfraConfig loader_config_;

    // Owns the reader together with the whole schema-cache stack it points into, so a test can build one on the stack
    // with every dependency kept alive.
    struct Harness {
        Harness(
            const std::filesystem::path &root,
            TextFormatter *format,
            const UserDiagnostics *diag,
            const InfraConfig *loader_config)
            : config{format, make_config_providers(root, format, diag)}, layout_metadata{root, format, diag},
              metatiles_header{root, format}, resolver{&config, &layout_metadata, &metatiles_header, format, diag},
              cache{root, &resolver, format, diag}, jasc_loader{format}, anim_json_parser{format},
              anim_code_parser{format, diag}, metadata_provider{root, format, diag},
              attributes_csv_loader{format, loader_config, diag}, reader{
                                                                      root,
                                                                      &cache,
                                                                      &png_rgba_loader,
                                                                      &png_indexed_loader,
                                                                      &jasc_loader,
                                                                      &attributes_csv_loader,
                                                                      &anim_json_parser,
                                                                      &anim_code_parser,
                                                                      &metadata_provider}
        {
        }

        [[nodiscard]] static std::vector<std::unique_ptr<ConfigProvider>>
        make_config_providers(const std::filesystem::path &root, TextFormatter *format, const UserDiagnostics *diag)
        {
            std::vector<std::unique_ptr<ConfigProvider>> providers{};
            providers.push_back(std::make_unique<YamlFileProvider>(format, diag, root));
            providers.push_back(std::make_unique<DefaultProvider>());
            return providers;
        }

        LazyLayeredConfig config;
        ProjectLayoutMetadataProvider layout_metadata;
        MetatilesHeaderProvider metatiles_header;
        TilesetAttributeSchemaResolver resolver;
        TilesetAttributeSchemaCache cache;
        PngRgbaImageLoader png_rgba_loader{};
        PngIndexedImageLoader png_indexed_loader{};
        JascPaletteLoader jasc_loader;
        AnimJsonParser anim_json_parser;
        AnimCodeParser anim_code_parser;
        ProjectTilesetMetadataProvider metadata_provider;
        AttributesCsvLoader attributes_csv_loader;
        ProjectTilesetArtifactReader reader;
    };

    void SetUp() override
    {
        // YamlFileProvider caches parsed files by path process-wide, so each test needs a distinct root.
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        project_root_ = std::filesystem::temp_directory_path() /
                        (std::string{"porytiles_artifact_reader_test_"} + info->test_suite_name() + "_" + info->name());
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

    void write_tileset_config(const std::string &tileset_name, const std::string &yaml)
    {
        const auto dir = project_root_ / "porytiles" / "tilesets" / tileset_name;
        std::filesystem::create_directories(dir);
        std::ofstream out{dir / "config.yaml"};
        out << yaml;
    }

    void write_file(const std::string &relative_path, const std::string &content)
    {
        const auto path = project_root_ / relative_path;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out{path};
        out << content;
    }

    void write_binary_file(const std::string &relative_path, const std::vector<std::uint8_t> &bytes)
    {
        const auto path = project_root_ / relative_path;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out{path, std::ios::binary};
        out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    [[nodiscard]] static Tileset make_tileset(const std::string &name)
    {
        return Tileset{
            name, std::make_unique<PorytilesTilesetComponent>(), std::make_unique<PorymapTilesetComponent>()};
    }

    [[nodiscard]] std::string error_text(const ChainableResult<void> &result)
    {
        std::string text;
        for (const auto &err : result.chain()) {
            text += err->join(formatter_);
            text += "\n";
        }
        return text;
    }
};

// The Issue 283 crux for the read path: a paired primary's artifacts must decode with the primary's own resolved
// schema, even while the command target (a secondary with divergent per-tileset config) has already resolved a
// different one.
TEST_F(ProjectTilesetArtifactReaderTest, PairedPrimaryArtifactsDecodeWithThePrimarysOwnSchema)
{
    write_config(kDivergentFieldsYaml);
    write_tileset_config("gTileset_Secondary", "fieldmap:\n  use_frlg_alternate_masks: always\n");
    write_file("data/tilesets/primary/test/attributes.csv", "id,behavior\n0,255\n");
    // One 2-byte attribute, value 0x01FF: the primary's behavior mask (0x00FF) reads 0xFF, the secondary's FRLG mask
    // (0x01FF) would read 0x1FF.
    write_binary_file("data/tilesets/primary/test/metatile_attributes.bin", {0xFF, 0x01});

    Harness harness{project_root_, &formatter_, &diag_, &loader_config_};

    // Simulate a secondary compile: the command target's schema resolves first, exactly the state in which the old
    // single-schema wiring decoded the primary with the secondary's schema.
    ASSERT_TRUE(harness.cache.entry("gTileset_Secondary").has_value());

    Tileset primary = make_tileset("gTileset_Primary");

    const auto csv_result =
        harness.reader.read_attributes_csv(primary, ArtifactKey{"data/tilesets/primary/test/attributes.csv"});
    ASSERT_TRUE(csv_result.has_value()) << error_text(csv_result);
    ASSERT_TRUE(primary.porytiles_component().metatile_attributes().contains(0));
    EXPECT_EQ(primary.porytiles_component().metatile_attributes().at(0).field("behavior"), 255U);

    const auto bin_result = harness.reader.read_metatile_attributes_bin(
        primary, ArtifactKey{"data/tilesets/primary/test/metatile_attributes.bin"});
    ASSERT_TRUE(bin_result.has_value()) << error_text(bin_result);
    ASSERT_EQ(primary.porymap_component().metatile_attributes_bin().size(), 1U);
    EXPECT_EQ(primary.porymap_component().metatile_attributes_bin().at(0).field("behavior"), 0xFFU);

    // Sanity check that the schemas really diverge: the same CSV shape fails under the secondary's name, because the
    // secondary's FRLG schema expects a terrain column.
    Tileset secondary = make_tileset("gTileset_Secondary");
    const auto secondary_csv =
        harness.reader.read_attributes_csv(secondary, ArtifactKey{"data/tilesets/primary/test/attributes.csv"});
    EXPECT_FALSE(secondary_csv.has_value());
}

} // namespace
} // namespace porytiles

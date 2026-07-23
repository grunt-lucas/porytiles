#include "porytiles/infra/repos/project_tileset_artifact_reader.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "porytiles/domain/config/role_pin_definition.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/infra/services/anim_code_parser.hpp"
#include "porytiles/infra/services/anim_json_parser.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
#include "porytiles/infra/services/jasc_palette_loader.hpp"
#include "porytiles/infra/services/png_indexed_image_loader.hpp"
#include "porytiles/infra/services/png_rgba_image_loader.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_infra_config.hpp"

using namespace porytiles;

namespace {

// Reader fixture wiring the concrete artifact-reader dependencies. Only read_attributes_csv is exercised here, so the
// other loaders/parsers are present just to satisfy the constructor; the interesting collaborators are the schema, the
// (empty) provider map, and the AttributesCsvLoader driven by a MockInfraConfig.
class ProjectTilesetArtifactReaderTest : public ::testing::Test {
  protected:
    std::filesystem::path project_root_;
    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
    MockInfraConfig config_{};
    Schema schema_ = std::move(Schema::create({Field{"behavior", 0x00FF}}, 2)).value();
    ProviderMap providers_{};
    PngRgbaImageLoader png_rgba_loader_{};
    PngIndexedImageLoader png_indexed_loader_{};
    JascPaletteLoader palette_loader_{&formatter_};
    AnimJsonParser anim_json_parser_{&formatter_};
    AnimCodeParser anim_code_parser_{&formatter_, &diag_};
    // read_attributes_csv never touches the metadata provider, so an empty project root here is fine.
    ProjectTilesetMetadataProvider metadata_provider_{std::filesystem::path{}, &formatter_, &diag_};
    AttributesCsvLoader attributes_csv_loader_{&formatter_, &config_, &diag_};

    void SetUp() override
    {
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        project_root_ = std::filesystem::temp_directory_path() /
                        (std::string{"porytiles_reader_test_"} + info->test_suite_name() + "_" + info->name());
        std::filesystem::remove_all(project_root_);
        std::filesystem::create_directories(project_root_);
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(project_root_, ec);
    }

    void write_csv(const std::string &contents)
    {
        std::ofstream out{project_root_ / "attributes.csv"};
        out << contents;
    }

    [[nodiscard]] ProjectTilesetArtifactReader make_reader() const
    {
        return ProjectTilesetArtifactReader{
            project_root_,
            &schema_,
            &providers_,
            &png_rgba_loader_,
            &png_indexed_loader_,
            &palette_loader_,
            &attributes_csv_loader_,
            &anim_json_parser_,
            &anim_code_parser_,
            &metadata_provider_};
    }

    [[nodiscard]] static Tileset make_dest()
    {
        return Tileset{
            "test_tileset", std::make_unique<PorytilesTilesetComponent>(), std::make_unique<PorymapTilesetComponent>()};
    }
};

// The layer_type role is pinned and the CSV carries its active pin column: the reader records column_present.
TEST_F(ProjectTilesetArtifactReaderTest, RecordsColumnPresentWhenActivePinColumnInCsv)
{
    config_.role_pins = RolePinDefinitions{{FieldRole::layer_type, std::nullopt}};
    write_csv("id,behavior,layer_type\n0,0,covered\n1,1,\n");

    Tileset dest = make_dest();
    const auto result = make_reader().read_attributes_csv(dest, ArtifactKey{"attributes.csv"});
    ASSERT_TRUE(result.has_value()) << result.error().join(formatter_);

    EXPECT_EQ(
        dest.porytiles_component().prior_pin_column_state(FieldRole::layer_type), PriorPinColumnState::column_present);
}

// The layer_type role is pinned but the CSV lacks the pin column: the reader records column_absent.
TEST_F(ProjectTilesetArtifactReaderTest, RecordsColumnAbsentWhenActivePinColumnMissing)
{
    config_.role_pins = RolePinDefinitions{{FieldRole::layer_type, std::nullopt}};
    write_csv("id,behavior\n0,0\n1,1\n");

    Tileset dest = make_dest();
    const auto result = make_reader().read_attributes_csv(dest, ArtifactKey{"attributes.csv"});
    ASSERT_TRUE(result.has_value()) << result.error().join(formatter_);

    EXPECT_EQ(
        dest.porytiles_component().prior_pin_column_state(FieldRole::layer_type), PriorPinColumnState::column_absent);
}

} // namespace

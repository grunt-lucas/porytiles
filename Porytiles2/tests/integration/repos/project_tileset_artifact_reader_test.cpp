#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles2/infra/services/anim_code_parser.hpp"
#include "porytiles2/infra/services/anim_yaml_parser.hpp"
#include "porytiles2/infra/services/attributes_csv_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

namespace {

Tileset create_empty_tileset(const std::string &name)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

/**
 * @brief A stub BehaviorMapProvider for testing that returns known values for common pokeemerald behaviors.
 */
class StubBehaviorMapProvider final : public BehaviorMapProvider {
  public:
    StubBehaviorMapProvider()
    {
        // Common pokeemerald behaviors
        name_to_value_["MB_NORMAL"] = 0x00;
        name_to_value_["MB_TALL_GRASS"] = 0x02;
        name_to_value_["MB_DEEP_WATER"] = 0x12;

        for (const auto &[name, value] : name_to_value_) {
            value_to_name_[value] = name;
        }
    }

    [[nodiscard]] ChainableResult<std::uint16_t> lookup(const std::string &behavior_name) const override
    {
        auto it = name_to_value_.find(behavior_name);
        if (it == name_to_value_.end()) {
            return FormattableError{"unknown behavior: {}", FormatParam{behavior_name}};
        }
        return it->second;
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint16_t behavior_value) const override
    {
        auto it = value_to_name_.find(behavior_value);
        if (it == value_to_name_.end()) {
            return FormattableError{"unknown behavior value: {}", FormatParam{behavior_value}};
        }
        return it->second;
    }

  private:
    std::unordered_map<std::string, std::uint16_t> name_to_value_{};
    std::unordered_map<std::uint16_t, std::string> value_to_name_{};
};

} // namespace

/**
 * @brief Base fixture for ProjectTilesetArtifactReader tests.
 *
 * @details
 * Subclass this fixture and override project_root_path() to test against different mock pokeemerald projects.
 */
class ProjectTilesetArtifactReaderTestBase : public ::testing::Test {
  protected:
    /**
     * @brief Returns the path to the mock pokeemerald project root.
     *
     * @details
     * Override this in derived fixtures to test against different project structures.
     */
    [[nodiscard]] virtual std::filesystem::path project_root_path() const = 0;

    void SetUp() override
    {
        project_root_ = project_root_path();

        ASSERT_TRUE(std::filesystem::exists(project_root_))
            << "Mock pokeemerald project not found at: " << project_root_;

        // Create formatter and diagnostics
        formatter_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<BufferedUserDiagnostics>();

        // Create loaders
        png_rgba_loader_ = std::make_unique<PngRgbaImageLoader>();
        png_indexed_loader_ = std::make_unique<PngIndexedImageLoader>();
        pal_loader_ = std::make_unique<JascPalLoader>(formatter_.get());
        behavior_map_ = std::make_unique<StubBehaviorMapProvider>();
        attributes_csv_loader_ = std::make_unique<AttributesCsvLoader>(formatter_.get(), behavior_map_.get());
        anim_yaml_parser_ = std::make_unique<AnimYamlParser>(formatter_.get());
        anim_code_parser_ = std::make_unique<AnimCodeParser>(formatter_.get(), diag_.get());
        metadata_provider_ =
            std::make_unique<ProjectTilesetMetadataProvider>(project_root_, formatter_.get(), diag_.get());

        // Create reader under test
        reader_ = std::make_unique<ProjectTilesetArtifactReader>(
            project_root_,
            png_rgba_loader_.get(),
            png_indexed_loader_.get(),
            pal_loader_.get(),
            attributes_csv_loader_.get(),
            anim_yaml_parser_.get(),
            anim_code_parser_.get(),
            metadata_provider_.get());
    }

    std::filesystem::path project_root_;
    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<PngRgbaImageLoader> png_rgba_loader_;
    std::unique_ptr<PngIndexedImageLoader> png_indexed_loader_;
    std::unique_ptr<JascPalLoader> pal_loader_;
    std::unique_ptr<StubBehaviorMapProvider> behavior_map_;
    std::unique_ptr<AttributesCsvLoader> attributes_csv_loader_;
    std::unique_ptr<AnimYamlParser> anim_yaml_parser_;
    std::unique_ptr<AnimCodeParser> anim_code_parser_;
    std::unique_ptr<ProjectTilesetMetadataProvider> metadata_provider_;
    std::unique_ptr<ProjectTilesetArtifactReader> reader_;
};

class ProjectTilesetArtifactReaderTest_Fixture1 : public ProjectTilesetArtifactReaderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "Resources/Tests/integration/repos/pokeemerald_vanilla_stock";
    }
};

TEST_F(ProjectTilesetArtifactReaderTest_Fixture1, ReadTilesPngPopulatesTilesetWithIndexedImage)
{
    // The mock project contains tiles.png at:
    // data/tilesets/primary/general/tiles.png
    auto tiles_path = project_root_ / "data/tilesets/primary/general/tiles.png";
    ASSERT_TRUE(std::filesystem::exists(tiles_path)) << "Test asset not found at: " << tiles_path;

    auto tileset = create_empty_tileset("gTileset_General");
    ArtifactKey key{"data/tilesets/primary/general/tiles.png"};

    auto result = reader_->read_tiles_png(tileset, key);
    ASSERT_TRUE(result.has_value()) << "Expected read_tiles_png to succeed";

    // Verify the tileset's porymap component now has the tiles image with valid dimensions
    const auto &porymap = tileset.porymap_component();
    const auto &tiles_png = porymap.tiles_png();

    // Tiles PNG should be 128 pixels wide (standard pokeemerald format), height varies by tileset size
    EXPECT_EQ(tiles_png.width(), 128) << "Tiles PNG should be 128 pixels wide";
    EXPECT_GT(tiles_png.height(), 0) << "Tiles PNG should have positive height";
}

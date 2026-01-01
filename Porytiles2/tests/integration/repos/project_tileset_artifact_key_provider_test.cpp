#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <string>

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

/**
 * @brief Base fixture for ProjectTilesetArtifactKeyProvider tests.
 *
 * @details
 * Subclass this fixture and override project_root_path() to test against different mock pokeemerald projects.
 */
class ProjectTilesetArtifactKeyProviderTestBase : public ::testing::Test {
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

        formatter_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<BufferedUserDiagnostics>();

        key_provider_ =
            std::make_unique<ProjectTilesetArtifactKeyProvider>(project_root_, formatter_.get(), diag_.get());
    }

    std::filesystem::path project_root_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<UserDiagnostics> diag_;
    std::unique_ptr<ProjectTilesetArtifactKeyProvider> key_provider_;
};

class ProjectTilesetArtifactKeyProviderTest_Fixture1 : public ProjectTilesetArtifactKeyProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "Resources/Tests/integration/repos/pokeemerald_vanilla_stock";
    }
};

class ProjectTilesetArtifactKeyProviderTest_Fixture2 : public ProjectTilesetArtifactKeyProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "Resources/Tests/integration/repos/pokeemerald_porytilestesttilesets";
    }
};

TEST_F(ProjectTilesetArtifactKeyProviderTest_Fixture1, KeyForRequiredArtifactsReturnsCorrectPathAndExists)
{
    // tiles.png
    auto tiles_png_result = key_provider_->key_for_tiles_png("gTileset_General");
    if (!tiles_png_result.has_value()) {
        std::string error_msg;
        for (const auto &err : tiles_png_result.chain()) {
            error_msg += err->join(*formatter_) + "\n";
        }
        FAIL() << "Expected key_for_tiles_png to succeed, but got error:\n" << error_msg;
    }
    const auto &tiles_png_key = tiles_png_result.value();
    EXPECT_TRUE(tiles_png_key.key().find("tiles.png") != std::string::npos)
        << "Key path should contain 'tiles.png', got: " << tiles_png_key.key();
    EXPECT_TRUE(key_provider_->artifact_exists(tiles_png_key)) << "Artifact should exist at: " << tiles_png_key.key();

    // metatiles.bin
    auto metatiles_bin_result = key_provider_->key_for_metatiles_bin("gTileset_General");
    if (!metatiles_bin_result.has_value()) {
        std::string error_msg;
        for (const auto &err : metatiles_bin_result.chain()) {
            error_msg += err->join(*formatter_) + "\n";
        }
        FAIL() << "Expected key_for_metatiles_bin to succeed, but got error:\n" << error_msg;
    }
    const auto &metatiles_bin_key = metatiles_bin_result.value();
    EXPECT_TRUE(metatiles_bin_key.key().find("metatiles.bin") != std::string::npos)
        << "Key path should contain 'metatiles.bin', got: " << metatiles_bin_key.key();
    EXPECT_TRUE(key_provider_->artifact_exists(metatiles_bin_key))
        << "Artifact should exist at: " << metatiles_bin_key.key();

    // metatile_attributes.bin
    auto metatile_attributes_bin_result = key_provider_->key_for_metatile_attributes_bin("gTileset_General");
    if (!metatile_attributes_bin_result.has_value()) {
        std::string error_msg;
        for (const auto &err : metatile_attributes_bin_result.chain()) {
            error_msg += err->join(*formatter_) + "\n";
        }
        FAIL() << "Expected key_for_metatile_attributes_bin to succeed, but got error:\n" << error_msg;
    }
    const auto &metatile_attributes_bin_key = metatile_attributes_bin_result.value();
    EXPECT_TRUE(metatile_attributes_bin_key.key().find("metatile_attributes.bin") != std::string::npos)
        << "Key path should contain 'metatile_attributes.bin', got: " << metatile_attributes_bin_key.key();
    EXPECT_TRUE(key_provider_->artifact_exists(metatile_attributes_bin_key))
        << "Artifact should exist at: " << metatile_attributes_bin_key.key();

    // palettes
    for (std::size_t i = 0; i < pal::num_pals; i++) {
        auto pal_result = key_provider_->key_for_porymap_pal_n("gTileset_General", i);
        if (!pal_result.has_value()) {
            std::string error_msg;
            for (const auto &err : pal_result.chain()) {
                error_msg += err->join(*formatter_) + "\n";
            }
            FAIL() << "Expected key_for_porymap_pal_n to succeed, but got error:\n" << error_msg;
        }
        const auto &pal_key = pal_result.value();
        EXPECT_TRUE(pal_key.key().find(std::to_string(i) + ".pal") != std::string::npos)
            << "Key path should contain '" + std::to_string(i) + ".pal', got: " << pal_key.key();
        EXPECT_TRUE(key_provider_->artifact_exists(pal_key)) << "Artifact should exist at: " << pal_key.key();
    }
}

TEST_F(ProjectTilesetArtifactKeyProviderTest_Fixture1, AnimKeysReturnCorrectPathAndExistsForGeneral)
{
    /*
     * Key provider should find all five gTileset_General tileset anims.
     */
    auto discovered_porymap_anims_result = key_provider_->discover_porymap_anims("gTileset_General");
    ASSERT_TRUE(discovered_porymap_anims_result.has_value());
    const auto &discovered_porymap_anims = discovered_porymap_anims_result.value();
    ASSERT_EQ(discovered_porymap_anims.size(), 5);
    EXPECT_TRUE(discovered_porymap_anims.contains("flower"));
    EXPECT_TRUE(discovered_porymap_anims.contains("water"));
    EXPECT_TRUE(discovered_porymap_anims.contains("sand_water_edge"));
    EXPECT_TRUE(discovered_porymap_anims.contains("land_water_edge"));
    EXPECT_TRUE(discovered_porymap_anims.contains("waterfall"));

    /*
     * Key provider should find all flower anim frames, and they should all exist.
     */
    auto discovered_porymap_flower_anim_frames_result =
        key_provider_->discover_porymap_anim_frames("gTileset_General", "flower");
    ASSERT_TRUE(discovered_porymap_flower_anim_frames_result.has_value());
    const auto &discovered_porymap_flower_anim_frames = discovered_porymap_flower_anim_frames_result.value();
    EXPECT_EQ(discovered_porymap_flower_anim_frames.size(), 3);
    EXPECT_TRUE(discovered_porymap_flower_anim_frames.contains("0"));
    EXPECT_TRUE(discovered_porymap_flower_anim_frames.contains("1"));
    EXPECT_TRUE(discovered_porymap_flower_anim_frames.contains("2"));

    auto porymap_flower_anim_frame_0_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_General", "flower", "0");
    ASSERT_TRUE(porymap_flower_anim_frame_0_key_result.has_value());
    const auto &porymap_flower_anim_frame_0_key = porymap_flower_anim_frame_0_key_result.value();
    EXPECT_TRUE(porymap_flower_anim_frame_0_key.key().contains("data/tilesets/primary/general/anim/flower/0.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_flower_anim_frame_0_key));

    auto porymap_flower_anim_frame_1_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_General", "flower", "1");
    ASSERT_TRUE(porymap_flower_anim_frame_1_key_result.has_value());
    const auto &porymap_flower_anim_frame_1_key = porymap_flower_anim_frame_1_key_result.value();
    EXPECT_TRUE(porymap_flower_anim_frame_1_key.key().contains("data/tilesets/primary/general/anim/flower/1.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_flower_anim_frame_1_key));

    auto porymap_flower_anim_frame_2_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_General", "flower", "2");
    ASSERT_TRUE(porymap_flower_anim_frame_2_key_result.has_value());
    const auto &porymap_flower_anim_frame_2_key = porymap_flower_anim_frame_2_key_result.value();
    EXPECT_TRUE(porymap_flower_anim_frame_2_key.key().contains("data/tilesets/primary/general/anim/flower/2.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_flower_anim_frame_2_key));

    /*
     * Key provider should find all land_water_edge anim frames, and they should all exist.
     */
    auto discovered_porymap_land_water_edge_anim_frames_result =
        key_provider_->discover_porymap_anim_frames("gTileset_General", "land_water_edge");
    ASSERT_TRUE(discovered_porymap_land_water_edge_anim_frames_result.has_value());
    const auto &discovered_porymap_land_water_edge_anim_frames =
        discovered_porymap_land_water_edge_anim_frames_result.value();
    EXPECT_EQ(discovered_porymap_land_water_edge_anim_frames.size(), 4);
    EXPECT_TRUE(discovered_porymap_land_water_edge_anim_frames.contains("0"));
    EXPECT_TRUE(discovered_porymap_land_water_edge_anim_frames.contains("1"));
    EXPECT_TRUE(discovered_porymap_land_water_edge_anim_frames.contains("2"));
    EXPECT_TRUE(discovered_porymap_land_water_edge_anim_frames.contains("3"));

    auto porymap_land_water_edge_anim_frame_0_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_General", "land_water_edge", "0");
    ASSERT_TRUE(porymap_land_water_edge_anim_frame_0_key_result.has_value());
    const auto &porymap_land_water_edge_anim_frame_0_key = porymap_land_water_edge_anim_frame_0_key_result.value();
    EXPECT_TRUE(porymap_land_water_edge_anim_frame_0_key.key().contains(
        "data/tilesets/primary/general/anim/land_water_edge/0.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_land_water_edge_anim_frame_0_key));

    auto porymap_land_water_edge_anim_frame_1_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_General", "land_water_edge", "1");
    ASSERT_TRUE(porymap_land_water_edge_anim_frame_1_key_result.has_value());
    const auto &porymap_land_water_edge_anim_frame_1_key = porymap_land_water_edge_anim_frame_1_key_result.value();
    EXPECT_TRUE(porymap_land_water_edge_anim_frame_1_key.key().contains(
        "data/tilesets/primary/general/anim/land_water_edge/1.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_land_water_edge_anim_frame_1_key));

    auto porymap_land_water_edge_anim_frame_2_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_General", "land_water_edge", "2");
    ASSERT_TRUE(porymap_land_water_edge_anim_frame_2_key_result.has_value());
    const auto &porymap_land_water_edge_anim_frame_2_key = porymap_land_water_edge_anim_frame_2_key_result.value();
    EXPECT_TRUE(porymap_land_water_edge_anim_frame_2_key.key().contains(
        "data/tilesets/primary/general/anim/land_water_edge/2.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_land_water_edge_anim_frame_2_key));

    auto porymap_land_water_edge_anim_frame_3_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_General", "land_water_edge", "3");
    ASSERT_TRUE(porymap_land_water_edge_anim_frame_3_key_result.has_value());
    const auto &porymap_land_water_edge_anim_frame_3_key = porymap_land_water_edge_anim_frame_3_key_result.value();
    EXPECT_TRUE(porymap_land_water_edge_anim_frame_3_key.key().contains(
        "data/tilesets/primary/general/anim/land_water_edge/3.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_land_water_edge_anim_frame_3_key));

    /*
     * Key provider should return path to generated header file, but it won't exist yet since this is a first-time
     * import.
     */
    auto anim_params_key_result = key_provider_->key_for_porymap_anim_params("gTileset_General");
    ASSERT_TRUE(anim_params_key_result.has_value());
    const auto &anim_params_key = anim_params_key_result.value();
    EXPECT_TRUE(anim_params_key.key().contains("data/tilesets/primary/general/include/generated_anim_code.h"));
    // First-time import, include/generated_anim_code.h does not exist yet
    EXPECT_FALSE(key_provider_->artifact_exists(anim_params_key));
}

TEST_F(ProjectTilesetArtifactKeyProviderTest_Fixture1, AnimKeysReturnCorrectPathAndExistsForBuilding)
{
    /*
     * Key provider should find the only gTileset_Building tileset anim.
     */
    auto discovered_porymap_anims_result = key_provider_->discover_porymap_anims("gTileset_Building");
    ASSERT_TRUE(discovered_porymap_anims_result.has_value());
    const auto &discovered_porymap_anims = discovered_porymap_anims_result.value();
    ASSERT_EQ(discovered_porymap_anims.size(), 1);
    EXPECT_TRUE(discovered_porymap_anims.contains("tv_turned_on"));

    /*
     * Key provider should find all tv_turned_on anim frames, and they should all exist.
     */
    auto discovered_porymap_tv_anim_frames_result =
        key_provider_->discover_porymap_anim_frames("gTileset_Building", "tv_turned_on");
    ASSERT_TRUE(discovered_porymap_tv_anim_frames_result.has_value());
    const auto &discovered_porymap_tv_anim_frames = discovered_porymap_tv_anim_frames_result.value();
    EXPECT_EQ(discovered_porymap_tv_anim_frames.size(), 2);
    EXPECT_TRUE(discovered_porymap_tv_anim_frames.contains("0"));
    EXPECT_TRUE(discovered_porymap_tv_anim_frames.contains("1"));

    auto porymap_tv_anim_frame_0_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_Building", "tv_turned_on", "0");
    ASSERT_TRUE(porymap_tv_anim_frame_0_key_result.has_value());
    const auto &porymap_tv_anim_frame_0_key = porymap_tv_anim_frame_0_key_result.value();
    EXPECT_TRUE(porymap_tv_anim_frame_0_key.key().contains("data/tilesets/primary/building/anim/tv_turned_on/0.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_tv_anim_frame_0_key));

    auto porymap_tv_anim_frame_1_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_Building", "tv_turned_on", "1");
    ASSERT_TRUE(porymap_tv_anim_frame_1_key_result.has_value());
    const auto &porymap_tv_anim_frame_1_key = porymap_tv_anim_frame_1_key_result.value();
    EXPECT_TRUE(porymap_tv_anim_frame_1_key.key().contains("data/tilesets/primary/building/anim/tv_turned_on/1.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_tv_anim_frame_1_key));

    /*
     * Key provider should return path to generated header file, but it won't exist yet since this is a first-time
     * import.
     */
    auto anim_params_key_result = key_provider_->key_for_porymap_anim_params("gTileset_Building");
    ASSERT_TRUE(anim_params_key_result.has_value());
    const auto &anim_params_key = anim_params_key_result.value();
    EXPECT_TRUE(anim_params_key.key().contains("data/tilesets/primary/building/include/generated_anim_code.h"));
    // First-time import, include/generated_anim_code.h does not exist yet
    EXPECT_FALSE(key_provider_->artifact_exists(anim_params_key));
}

TEST_F(ProjectTilesetArtifactKeyProviderTest_Fixture1, AnimKeysReturnNotExistsForSecretBase)
{
    /*
     * Key provider should find no gTileset_SecretBase tileset anims.
     */
    auto discovered_porymap_anims_result = key_provider_->discover_porymap_anims("gTileset_SecretBase");
    ASSERT_TRUE(discovered_porymap_anims_result.has_value());
    const auto &discovered_porymap_anims = discovered_porymap_anims_result.value();
    EXPECT_TRUE(discovered_porymap_anims.empty());
}

TEST_F(ProjectTilesetArtifactKeyProviderTest_Fixture2, KeyForRequiredArtifactsReturnsCorrectPathAndExists)
{
    // tiles.png
    auto tiles_png_result = key_provider_->key_for_tiles_png("gTileset_PorytilesTest");
    if (!tiles_png_result.has_value()) {
        std::string error_msg;
        for (const auto &err : tiles_png_result.chain()) {
            error_msg += err->join(*formatter_) + "\n";
        }
        FAIL() << "Expected key_for_tiles_png to succeed, but got error:\n" << error_msg;
    }
    const auto &tiles_png_key = tiles_png_result.value();
    EXPECT_TRUE(tiles_png_key.key().find("tiles.png") != std::string::npos)
        << "Key path should contain 'tiles.png', got: " << tiles_png_key.key();
    EXPECT_TRUE(key_provider_->artifact_exists(tiles_png_key)) << "Artifact should exist at: " << tiles_png_key.key();

    // metatiles.bin
    auto metatiles_bin_result = key_provider_->key_for_metatiles_bin("gTileset_PorytilesTest");
    if (!metatiles_bin_result.has_value()) {
        std::string error_msg;
        for (const auto &err : metatiles_bin_result.chain()) {
            error_msg += err->join(*formatter_) + "\n";
        }
        FAIL() << "Expected key_for_metatiles_bin to succeed, but got error:\n" << error_msg;
    }
    const auto &metatiles_bin_key = metatiles_bin_result.value();
    EXPECT_TRUE(metatiles_bin_key.key().find("metatiles.bin") != std::string::npos)
        << "Key path should contain 'metatiles.bin', got: " << metatiles_bin_key.key();
    EXPECT_TRUE(key_provider_->artifact_exists(metatiles_bin_key))
        << "Artifact should exist at: " << metatiles_bin_key.key();

    // metatile_attributes.bin
    auto metatile_attributes_bin_result = key_provider_->key_for_metatile_attributes_bin("gTileset_PorytilesTest");
    if (!metatile_attributes_bin_result.has_value()) {
        std::string error_msg;
        for (const auto &err : metatile_attributes_bin_result.chain()) {
            error_msg += err->join(*formatter_) + "\n";
        }
        FAIL() << "Expected key_for_metatile_attributes_bin to succeed, but got error:\n" << error_msg;
    }
    const auto &metatile_attributes_bin_key = metatile_attributes_bin_result.value();
    EXPECT_TRUE(metatile_attributes_bin_key.key().find("metatile_attributes.bin") != std::string::npos)
        << "Key path should contain 'metatile_attributes.bin', got: " << metatile_attributes_bin_key.key();
    EXPECT_TRUE(key_provider_->artifact_exists(metatile_attributes_bin_key))
        << "Artifact should exist at: " << metatile_attributes_bin_key.key();

    // palettes
    for (std::size_t i = 0; i < pal::num_pals; i++) {
        auto pal_result = key_provider_->key_for_porymap_pal_n("gTileset_PorytilesTest", i);
        if (!pal_result.has_value()) {
            std::string error_msg;
            for (const auto &err : pal_result.chain()) {
                error_msg += err->join(*formatter_) + "\n";
            }
            FAIL() << "Expected key_for_porymap_pal_n to succeed, but got error:\n" << error_msg;
        }
        const auto &pal_key = pal_result.value();
        EXPECT_TRUE(pal_key.key().find(std::to_string(i) + ".pal") != std::string::npos)
            << "Key path should contain '" + std::to_string(i) + ".pal', got: " << pal_key.key();
        EXPECT_TRUE(key_provider_->artifact_exists(pal_key)) << "Artifact should exist at: " << pal_key.key();
    }
}

TEST_F(ProjectTilesetArtifactKeyProviderTest_Fixture2, AnimKeysReturnCorrectPathAndExistsForPorytilesTest)
{
    /*
     * Key provider should find the only gTileset_PorytilesTest anim in both Porymap and Porytiles components.
     */
    auto discovered_porymap_anims_result = key_provider_->discover_porymap_anims("gTileset_PorytilesTest");
    ASSERT_TRUE(discovered_porymap_anims_result.has_value());
    const auto &discovered_porymap_anims = discovered_porymap_anims_result.value();
    ASSERT_EQ(discovered_porymap_anims.size(), 1);
    EXPECT_TRUE(discovered_porymap_anims.contains("flower_yellow"));

    auto discovered_porytiles_anims_result = key_provider_->discover_porytiles_anims("gTileset_PorytilesTest");
    ASSERT_TRUE(discovered_porytiles_anims_result.has_value());
    const auto &discovered_porytiles_anims = discovered_porymap_anims_result.value();
    ASSERT_EQ(discovered_porytiles_anims.size(), 1);
    EXPECT_TRUE(discovered_porytiles_anims.contains("flower_yellow"));

    /*
     * Key provider should find all flower_yellow anim frames, and they should all exist.
     */
    auto discovered_porymap_flower_anim_frames_result =
        key_provider_->discover_porymap_anim_frames("gTileset_PorytilesTest", "flower_yellow");
    ASSERT_TRUE(discovered_porymap_flower_anim_frames_result.has_value());
    const auto &discovered_porymap_flower_anim_frames = discovered_porymap_flower_anim_frames_result.value();
    EXPECT_EQ(discovered_porymap_flower_anim_frames.size(), 3);
    EXPECT_TRUE(discovered_porymap_flower_anim_frames.contains("center"));
    EXPECT_TRUE(discovered_porymap_flower_anim_frames.contains("left"));
    EXPECT_TRUE(discovered_porymap_flower_anim_frames.contains("right"));

    auto porymap_flower_anim_frame_center_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_PorytilesTest", "flower_yellow", "center");
    ASSERT_TRUE(porymap_flower_anim_frame_center_key_result.has_value());
    const auto &porymap_flower_anim_frame_center_key = porymap_flower_anim_frame_center_key_result.value();
    EXPECT_TRUE(porymap_flower_anim_frame_center_key.key().contains(
        "data/tilesets/primary/porytiles_test/anim/flower_yellow/center.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_flower_anim_frame_center_key));

    auto porymap_flower_anim_frame_left_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_PorytilesTest", "flower_yellow", "left");
    ASSERT_TRUE(porymap_flower_anim_frame_left_key_result.has_value());
    const auto &porymap_flower_anim_frame_left_key = porymap_flower_anim_frame_left_key_result.value();
    EXPECT_TRUE(porymap_flower_anim_frame_left_key.key().contains(
        "data/tilesets/primary/porytiles_test/anim/flower_yellow/left.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_flower_anim_frame_left_key));

    auto porymap_flower_anim_frame_right_key_result =
        key_provider_->key_for_porymap_anim_frame("gTileset_PorytilesTest", "flower_yellow", "right");
    ASSERT_TRUE(porymap_flower_anim_frame_right_key_result.has_value());
    const auto &porymap_flower_anim_frame_right_key = porymap_flower_anim_frame_right_key_result.value();
    EXPECT_TRUE(porymap_flower_anim_frame_right_key.key().contains(
        "data/tilesets/primary/porytiles_test/anim/flower_yellow/right.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_flower_anim_frame_right_key));

    auto discovered_porytiles_flower_anim_frames_result =
        key_provider_->discover_porytiles_anim_frames("gTileset_PorytilesTest", "flower_yellow");
    ASSERT_TRUE(discovered_porytiles_flower_anim_frames_result.has_value());
    const auto &discovered_porytiles_flower_anim_frames = discovered_porytiles_flower_anim_frames_result.value();
    EXPECT_EQ(discovered_porytiles_flower_anim_frames.size(), 4);
    EXPECT_TRUE(discovered_porytiles_flower_anim_frames.contains("key"));
    EXPECT_TRUE(discovered_porytiles_flower_anim_frames.contains("center"));
    EXPECT_TRUE(discovered_porytiles_flower_anim_frames.contains("left"));
    EXPECT_TRUE(discovered_porytiles_flower_anim_frames.contains("right"));

    auto porytiles_flower_anim_frame_key_key_result =
        key_provider_->key_for_porytiles_anim_frame("gTileset_PorytilesTest", "flower_yellow", "key");
    ASSERT_TRUE(porytiles_flower_anim_frame_key_key_result.has_value());
    const auto &porytiles_flower_anim_frame_key_key = porytiles_flower_anim_frame_key_key_result.value();
    EXPECT_TRUE(porytiles_flower_anim_frame_key_key.key().contains(
        "data/tilesets/primary/porytiles_test/porytiles/anim/flower_yellow/key.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porytiles_flower_anim_frame_key_key));

    auto porytiles_flower_anim_frame_center_key_result =
        key_provider_->key_for_porytiles_anim_frame("gTileset_PorytilesTest", "flower_yellow", "center");
    ASSERT_TRUE(porytiles_flower_anim_frame_center_key_result.has_value());
    const auto &porytiles_flower_anim_frame_center_key = porytiles_flower_anim_frame_center_key_result.value();
    EXPECT_TRUE(porytiles_flower_anim_frame_center_key.key().contains(
        "data/tilesets/primary/porytiles_test/porytiles/anim/flower_yellow/center.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porytiles_flower_anim_frame_center_key));

    auto porytiles_flower_anim_frame_left_key_result =
        key_provider_->key_for_porytiles_anim_frame("gTileset_PorytilesTest", "flower_yellow", "left");
    ASSERT_TRUE(porytiles_flower_anim_frame_left_key_result.has_value());
    const auto &porytiles_flower_anim_frame_left_key = porytiles_flower_anim_frame_left_key_result.value();
    EXPECT_TRUE(porytiles_flower_anim_frame_left_key.key().contains(
        "data/tilesets/primary/porytiles_test/porytiles/anim/flower_yellow/left.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porytiles_flower_anim_frame_left_key));

    auto porytiles_flower_anim_frame_right_key_result =
        key_provider_->key_for_porytiles_anim_frame("gTileset_PorytilesTest", "flower_yellow", "right");
    ASSERT_TRUE(porytiles_flower_anim_frame_right_key_result.has_value());
    const auto &porytiles_flower_anim_frame_right_key = porytiles_flower_anim_frame_right_key_result.value();
    EXPECT_TRUE(porytiles_flower_anim_frame_right_key.key().contains(
        "data/tilesets/primary/porytiles_test/porytiles/anim/flower_yellow/right.png"));
    EXPECT_TRUE(key_provider_->artifact_exists(porytiles_flower_anim_frame_right_key));

    /*
     * Key provider should return path to generated header file, and it should exist since this is an already-onboarded
     * tileset. It should also find the Porytiles params file (anim.yaml).
     */
    auto porymap_anim_params_key_result = key_provider_->key_for_porymap_anim_params("gTileset_PorytilesTest");
    ASSERT_TRUE(porymap_anim_params_key_result.has_value());
    const auto &porymap_anim_params_key = porymap_anim_params_key_result.value();
    EXPECT_TRUE(
        porymap_anim_params_key.key().contains("data/tilesets/primary/porytiles_test/include/generated_anim_code.h"));
    EXPECT_TRUE(key_provider_->artifact_exists(porymap_anim_params_key));

    auto porytiles_anim_params_key_result = key_provider_->key_for_porytiles_anim_params("gTileset_PorytilesTest");
    ASSERT_TRUE(porytiles_anim_params_key_result.has_value());
    const auto &porytiles_anim_params_key = porytiles_anim_params_key_result.value();
    EXPECT_TRUE(
        porytiles_anim_params_key.key().contains("data/tilesets/primary/porytiles_test/porytiles/anim/anim.yaml"));
    EXPECT_TRUE(key_provider_->artifact_exists(porytiles_anim_params_key));
}

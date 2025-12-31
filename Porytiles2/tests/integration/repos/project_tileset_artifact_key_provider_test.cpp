#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <string>

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_metadata_provider.hpp"
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

        metadata_provider_ =
            std::make_unique<ProjectTilesetMetadataProvider>(project_root_, formatter_.get(), diag_.get());

        key_provider_ = std::make_unique<ProjectTilesetArtifactKeyProvider>(
            project_root_, metadata_provider_.get(), formatter_.get(), diag_.get());
    }

    std::filesystem::path project_root_;
    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<ProjectTilesetMetadataProvider> metadata_provider_;
    std::unique_ptr<ProjectTilesetArtifactKeyProvider> key_provider_;
};

/**
 * @brief Tests using the pokeemerald_general_firsttimeimport_success mock project.
 */
class ProjectTilesetArtifactKeyProviderTest_Fixture1 : public ProjectTilesetArtifactKeyProviderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path project_root_path() const override
    {
        return "Resources/Tests/integration/repos/pokeemerald_general_firsttimeimport_success";
    }
};

TEST_F(ProjectTilesetArtifactKeyProviderTest_Fixture1, KeyForTilesPngReturnsCorrectPathAndExists)
{
    // The mock project contains gTileset_General with tiles at:
    // data/tilesets/primary/general/tiles.png

    auto result = key_provider_->key_for_tiles_png("gTileset_General");
    if (!result.has_value()) {
        std::string error_msg;
        for (const auto &err : result.chain()) {
            error_msg += err->join(*formatter_) + "\n";
        }
        FAIL() << "Expected key_for_tiles_png to succeed, but got error:\n" << error_msg;
    }

    const auto &key = result.value();

    // The key should contain a path that includes "tiles.png"
    EXPECT_TRUE(key.key().find("tiles.png") != std::string::npos)
        << "Key path should contain 'tiles.png', got: " << key.key();

    // Verify the artifact actually exists on disk
    EXPECT_TRUE(key_provider_->artifact_exists(key)) << "Artifact should exist at: " << key.key();
}

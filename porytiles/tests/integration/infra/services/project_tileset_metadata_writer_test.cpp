#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "porytiles/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

namespace {

[[nodiscard]] std::string read_file_contents(const std::filesystem::path &path)
{
    std::ifstream file{path};
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void copy_directory(const std::filesystem::path &src, const std::filesystem::path &dst)
{
    std::filesystem::create_directories(dst);
    for (const auto &entry : std::filesystem::recursive_directory_iterator(src)) {
        const auto relative_path = std::filesystem::relative(entry.path(), src);
        const auto target_path = dst / relative_path;

        if (entry.is_directory()) {
            std::filesystem::create_directories(target_path);
        }
        else {
            std::filesystem::copy_file(entry.path(), target_path, std::filesystem::copy_options::overwrite_existing);
        }
    }
}

} // namespace

/**
 * @brief Base fixture for ProjectTilesetMetadataWriter tests.
 *
 * @details
 * This fixture copies the test project to a temp directory so we can modify files
 * without affecting the original test data.
 */
class ProjectTilesetMetadataWriterTestBase : public ::testing::Test {
  protected:
    [[nodiscard]] virtual std::filesystem::path source_project_path() const = 0;

    void SetUp() override
    {
        // Create a unique temp directory for this test
        temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_writer_test";
        std::filesystem::remove_all(temp_dir_);
        std::filesystem::create_directories(temp_dir_);

        // Copy the source project to temp
        copy_directory(source_project_path(), temp_dir_);

        ASSERT_TRUE(std::filesystem::exists(temp_dir_)) << "Temp project directory not created at: " << temp_dir_;

        formatter_ = std::make_unique<PlainTextFormatter>();
        writer_ = std::make_unique<ProjectTilesetMetadataWriter>(temp_dir_, formatter_.get());
    }

    void TearDown() override
    {
        // Clean up temp directory
        std::filesystem::remove_all(temp_dir_);
    }

    [[nodiscard]] std::filesystem::path headers_path() const
    {
        return temp_dir_ / "src" / "data" / "tilesets" / "headers.h";
    }

    std::filesystem::path temp_dir_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<ProjectTilesetMetadataWriter> writer_;
};

class ProjectTilesetMetadataWriterTest_Fixture1 : public ProjectTilesetMetadataWriterTestBase {
  protected:
    [[nodiscard]] std::filesystem::path source_project_path() const override
    {
        return "resources/tests/integration/shared/repos/pokeemerald_porytilestesttilesets";
    }
};

TEST_F(ProjectTilesetMetadataWriterTest_Fixture1, UpdatesCallbackFromFunctionToNull)
{
    // gTileset_General has .callback = InitTilesetAnim_General
    // Update it to NULL
    auto result = writer_->update_callback("gTileset_General", "NULL");
    ASSERT_TRUE(result.has_value()) << "update_callback failed";

    // Read the file and verify the change
    std::string contents = read_file_contents(headers_path());
    EXPECT_TRUE(contents.find("    .callback = NULL,") != std::string::npos)
        << "Expected to find '    .callback = NULL,' in file";

    // Verify the old value is gone
    EXPECT_TRUE(contents.find("InitTilesetAnim_General") == std::string::npos)
        << "Expected InitTilesetAnim_General to be removed";
}

TEST_F(ProjectTilesetMetadataWriterTest_Fixture1, UpdatesCallbackFromNullToFunction)
{
    // gTileset_Shop has .callback = NULL
    // Update it to a function
    auto result = writer_->update_callback("gTileset_Shop", "InitTilesetAnim_Shop");
    ASSERT_TRUE(result.has_value()) << "update_callback failed";

    // Read the file and verify the change
    std::string contents = read_file_contents(headers_path());
    EXPECT_TRUE(contents.find("    .callback = InitTilesetAnim_Shop,") != std::string::npos)
        << "Expected to find '    .callback = InitTilesetAnim_Shop,' in file";
}

TEST_F(ProjectTilesetMetadataWriterTest_Fixture1, UpdatesCallbackFromFunctionToFunction)
{
    // gTileset_General has .callback = InitTilesetAnim_General
    // Update it to a different function
    auto result = writer_->update_callback("gTileset_General", "InitTilesetAnim_PorytilesManaged_General");
    ASSERT_TRUE(result.has_value()) << "update_callback failed";

    // Read the file and verify the change
    std::string contents = read_file_contents(headers_path());
    EXPECT_TRUE(contents.find("    .callback = InitTilesetAnim_PorytilesManaged_General,") != std::string::npos)
        << "Expected to find the new callback value in file";

    // Verify the old value is gone
    EXPECT_TRUE(contents.find("InitTilesetAnim_General,") == std::string::npos)
        << "Expected old callback to be removed";
}

TEST_F(ProjectTilesetMetadataWriterTest_Fixture1, ReturnsErrorForNonexistentTileset)
{
    auto result = writer_->update_callback("gTileset_DoesNotExist", "NULL");
    EXPECT_FALSE(result.has_value()) << "Expected error for nonexistent tileset";
}

TEST_F(ProjectTilesetMetadataWriterTest_Fixture1, PreservesOtherTilesetsUnchanged)
{
    // Read original file
    std::string original_contents = read_file_contents(headers_path());

    // Update only gTileset_General
    auto result = writer_->update_callback("gTileset_General", "NewCallback");
    ASSERT_TRUE(result.has_value()) << "update_callback failed";

    // Read updated file
    std::string updated_contents = read_file_contents(headers_path());

    // Verify other tilesets are unchanged - gTileset_Shop should still have NULL
    EXPECT_TRUE(updated_contents.find("gTileset_Shop") != std::string::npos) << "gTileset_Shop should still be present";

    // Count occurrences of gTileset_ - should be the same
    std::size_t original_count = 0;
    std::size_t updated_count = 0;
    std::string search = "gTileset_";
    std::size_t pos = 0;
    while ((pos = original_contents.find(search, pos)) != std::string::npos) {
        ++original_count;
        pos += search.length();
    }
    pos = 0;
    while ((pos = updated_contents.find(search, pos)) != std::string::npos) {
        ++updated_count;
        pos += search.length();
    }
    EXPECT_EQ(original_count, updated_count) << "Number of tilesets should be unchanged";
}

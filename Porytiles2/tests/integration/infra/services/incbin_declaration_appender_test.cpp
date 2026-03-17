#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "porytiles2/infra/services/incbin_declaration_appender.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

using namespace porytiles2;

namespace {

/**
 * @brief Reads the entire contents of a file into a string.
 */
[[nodiscard]] std::string read_file_contents(const std::filesystem::path &path)
{
    std::ifstream file{path};
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief Copies a directory recursively.
 */
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
 * @brief Base fixture for IncbinDeclarationAppender tests.
 *
 * @details
 * This fixture copies the test project to a temp directory so we can modify files
 * without affecting the original test data.
 */
class IncbinDeclarationAppenderTestBase : public ::testing::Test {
  protected:
    [[nodiscard]] virtual std::filesystem::path source_project_path() const = 0;

    void SetUp() override
    {
        // Create a unique temp directory for this test
        temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_incbin_appender_test";
        std::filesystem::remove_all(temp_dir_);
        std::filesystem::create_directories(temp_dir_);

        // Copy the source project to temp
        copy_directory(source_project_path(), temp_dir_);

        ASSERT_TRUE(std::filesystem::exists(temp_dir_)) << "Temp project directory not created at: " << temp_dir_;

        formatter_ = std::make_unique<PlainTextFormatter>();
        appender_ = std::make_unique<IncbinDeclarationAppender>(temp_dir_, formatter_.get());
    }

    void TearDown() override
    {
        // Clean up temp directory
        std::filesystem::remove_all(temp_dir_);
    }

    [[nodiscard]] std::filesystem::path graphics_path() const
    {
        return temp_dir_ / "src" / "data" / "tilesets" / "graphics.h";
    }

    [[nodiscard]] std::filesystem::path metatiles_path() const
    {
        return temp_dir_ / "src" / "data" / "tilesets" / "metatiles.h";
    }

    std::filesystem::path temp_dir_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<IncbinDeclarationAppender> appender_;
};

/**
 * @brief Tests using the pokeemerald_vanilla_stock mock project.
 */
class IncbinDeclarationAppenderTest_VanillaStock : public IncbinDeclarationAppenderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path source_project_path() const override
    {
        return "Resources/Tests/integration/shared/repos/pokeemerald_vanilla_stock";
    }
};

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, AppendsMetatilesDeclarations)
{
    auto result = appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2);
    ASSERT_TRUE(result.has_value()) << "append_metatiles_declarations failed";

    const std::string content = read_file_contents(metatiles_path());

    // Verify the new declarations were appended
    EXPECT_NE(content.find("gMetatiles_PorytilesManaged_General"), std::string::npos)
        << "gMetatiles_PorytilesManaged_General not found in metatiles.h";
    EXPECT_NE(content.find("gMetatileAttributes_PorytilesManaged_General"), std::string::npos)
        << "gMetatileAttributes_PorytilesManaged_General not found in metatiles.h";

    // Verify the paths are correct
    EXPECT_NE(content.find("data/tilesets/primary/general/porytiles_bin/metatiles.bin"), std::string::npos)
        << "metatiles.bin path not found";
    EXPECT_NE(content.find("data/tilesets/primary/general/porytiles_bin/metatile_attributes.bin"), std::string::npos)
        << "metatile_attributes.bin path not found";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, AppendsGraphicsDeclarations)
{
    auto result = appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6);
    ASSERT_TRUE(result.has_value()) << "append_graphics_declarations failed";

    const std::string content = read_file_contents(graphics_path());

    // Verify the tiles declaration was appended
    EXPECT_NE(content.find("gTilesetTiles_PorytilesManaged_General"), std::string::npos)
        << "gTilesetTiles_PorytilesManaged_General not found in graphics.h";

    // Verify the palettes declaration was appended
    EXPECT_NE(content.find("gTilesetPalettes_PorytilesManaged_General"), std::string::npos)
        << "gTilesetPalettes_PorytilesManaged_General not found in graphics.h";

    // Verify the paths are correct
    EXPECT_NE(content.find("data/tilesets/primary/general/porytiles_bin/tiles.4bpp.lz"), std::string::npos)
        << "tiles.4bpp.lz path not found";
    EXPECT_NE(content.find("data/tilesets/primary/general/porytiles_bin/palettes/00.gbapal"), std::string::npos)
        << "palettes/00.gbapal path not found";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, PreservesExistingContent)
{
    // Get original content
    const std::string original_content = read_file_contents(metatiles_path());
    ASSERT_FALSE(original_content.empty()) << "Original metatiles.h is empty";

    // Append new declarations
    auto result = appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2);
    ASSERT_TRUE(result.has_value()) << "append_metatiles_declarations failed";

    const std::string new_content = read_file_contents(metatiles_path());

    // Verify original content is still present
    EXPECT_NE(new_content.find("gMetatiles_General"), std::string::npos) << "Original gMetatiles_General was removed";
    EXPECT_NE(new_content.find("gMetatileAttributes_General"), std::string::npos)
        << "Original gMetatileAttributes_General was removed";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, RemovesDeclarations)
{
    // First append
    auto append_result = appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2);
    ASSERT_TRUE(append_result.has_value()) << "append_metatiles_declarations failed";

    // Verify declarations were added
    std::string content = read_file_contents(metatiles_path());
    ASSERT_NE(content.find("gMetatiles_PorytilesManaged_General"), std::string::npos);

    // Now remove
    auto remove_result = appender_->remove_declarations("gTileset_General");
    ASSERT_TRUE(remove_result.has_value()) << "remove_declarations failed";

    // Verify declarations were removed
    content = read_file_contents(metatiles_path());
    EXPECT_EQ(content.find("gMetatiles_PorytilesManaged_General"), std::string::npos)
        << "gMetatiles_PorytilesManaged_General was not removed";
    EXPECT_EQ(content.find("gMetatileAttributes_PorytilesManaged_General"), std::string::npos)
        << "gMetatileAttributes_PorytilesManaged_General was not removed";

    // Verify original content is still present
    EXPECT_NE(content.find("gMetatiles_General"), std::string::npos)
        << "Original gMetatiles_General was incorrectly removed";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, RejectsInvalidTilesetName)
{
    auto result = appender_->append_metatiles_declarations("InvalidTileset", "data/tilesets/primary", 2);
    EXPECT_FALSE(result.has_value()) << "Should reject tileset name not starting with gTileset_";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, GeneratesCorrectSnakeCasePath)
{
    // Use a tileset with PascalCase shorthand to test snake_case conversion
    auto result = appender_->append_metatiles_declarations("gTileset_BattleFrontier", "data/tilesets/secondary", 2);
    ASSERT_TRUE(result.has_value()) << "append_metatiles_declarations failed";

    const std::string content = read_file_contents(metatiles_path());

    // Verify the path uses snake_case
    EXPECT_NE(content.find("data/tilesets/secondary/battle_frontier/porytiles_bin/metatiles.bin"), std::string::npos)
        << "snake_case path conversion not working correctly";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, AppendsMetatilesDeclarationsWithU16AttrSize)
{
    auto result = appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2);
    ASSERT_TRUE(result.has_value()) << "append_metatiles_declarations failed";

    const std::string content = read_file_contents(metatiles_path());

    // Verify u16 type and INCBIN_U16 macro for attr size 2
    EXPECT_NE(content.find("const u16 gMetatileAttributes_PorytilesManaged_General"), std::string::npos)
        << "const u16 attribute declaration not found";
    EXPECT_NE(content.find("INCBIN_U16"), std::string::npos) << "INCBIN_U16 not found for attr size 2";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, AppendsMetatilesDeclarationsWithU32AttrSize)
{
    auto result = appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 4);
    ASSERT_TRUE(result.has_value()) << "append_metatiles_declarations failed";

    const std::string content = read_file_contents(metatiles_path());

    // Verify u32 type and INCBIN_U32 macro for attr size 4
    EXPECT_NE(content.find("const u32 gMetatileAttributes_PorytilesManaged_General"), std::string::npos)
        << "const u32 attribute declaration not found for attr size 4";
    EXPECT_NE(content.find("INCBIN_U32"), std::string::npos) << "INCBIN_U32 not found for attr size 4";
}

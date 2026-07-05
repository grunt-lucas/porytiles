#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "porytiles/infra/services/incbin_declaration_appender.hpp"
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

[[nodiscard]] std::size_t count_occurrences(const std::string &haystack, const std::string &needle)
{
    std::size_t count = 0;
    for (std::size_t pos = haystack.find(needle); pos != std::string::npos; pos = haystack.find(needle, pos + 1)) {
        ++count;
    }
    return count;
}

// Simulates damage from the old find_append_position bug by parking a line just inside the
// file's trailing preprocessor conditional (immediately before the last #endif).
void insert_line_before_last_endif(const std::filesystem::path &path, const std::string &line)
{
    const std::string content = read_file_contents(path);
    const std::size_t pos = content.rfind("#endif");
    ASSERT_NE(pos, std::string::npos) << "fixture has no #endif: " << path;
    std::ofstream out{path};
    out << content.substr(0, pos) << line << "\n" << content.substr(pos);
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

class IncbinDeclarationAppenderTest_VanillaStock : public IncbinDeclarationAppenderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path source_project_path() const override
    {
        return "resources/tests/integration/shared/repos/pokeemerald_vanilla_stock";
    }
};

/**
 * @brief Fixture whose graphics.h and metatiles.h end in a trailing preprocessor conditional.
 *
 * @details
 * Mirrors pokeemerald-expansion, whose graphics.h ends with `#if IS_FRLG ... #endif` and whose
 * metatiles.h ends with `#if !IS_FRLG ... #else ... #endif`. Declarations must land after the final
 * `#endif` (preprocessor depth 0), not inside the FRLG-only branch.
 */
class IncbinDeclarationAppenderTest_ExpansionFrlgStock : public IncbinDeclarationAppenderTestBase {
  protected:
    [[nodiscard]] std::filesystem::path source_project_path() const override
    {
        return "resources/tests/integration/shared/repos/pokeemerald_expansion_frlg_stock";
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

TEST_F(IncbinDeclarationAppenderTest_ExpansionFrlgStock, AppendsGraphicsDeclarationsAfterFrlgBlock)
{
    auto result = appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6);
    ASSERT_TRUE(result.has_value()) << "append_graphics_declarations failed";

    const std::string content = read_file_contents(graphics_path());

    // The managed declaration must land after the trailing #endif, not inside the #if IS_FRLG block.
    const std::size_t decl_pos = content.find("gTilesetTiles_PorytilesManaged_General");
    ASSERT_NE(decl_pos, std::string::npos) << "managed tiles declaration not found";
    EXPECT_LT(content.rfind("#endif"), decl_pos) << "managed declaration landed inside the trailing FRLG block";

    // FRLG content is untouched.
    EXPECT_NE(content.find("gTilesetTiles_Building_Frlg"), std::string::npos) << "FRLG tiles declaration was disturbed";
    EXPECT_NE(content.find("#endif // IS_FRLG"), std::string::npos) << "FRLG #endif was disturbed";
}

TEST_F(IncbinDeclarationAppenderTest_ExpansionFrlgStock, AppendsMetatilesDeclarationsAfterFrlgBlock)
{
    auto result = appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2);
    ASSERT_TRUE(result.has_value()) << "append_metatiles_declarations failed";

    const std::string content = read_file_contents(metatiles_path());

    // The managed declaration must land after the trailing #endif, not inside the #if !IS_FRLG block.
    const std::size_t decl_pos = content.find("gMetatiles_PorytilesManaged_General");
    ASSERT_NE(decl_pos, std::string::npos) << "managed metatiles declaration not found";
    EXPECT_LT(content.rfind("#endif"), decl_pos) << "managed declaration landed inside the trailing FRLG block";

    // FRLG content is untouched.
    EXPECT_NE(content.find("gMetatiles_Building_Frlg"), std::string::npos)
        << "FRLG metatiles declaration was disturbed";
    EXPECT_NE(content.find("#endif // IS_FRLG"), std::string::npos) << "FRLG #endif was disturbed";
}

TEST_F(IncbinDeclarationAppenderTest_ExpansionFrlgStock, RepeatedAppendYieldsSingleDeclaration)
{
    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6).has_value());
    ASSERT_TRUE(appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2).has_value());

    const std::string graphics_after_first = read_file_contents(graphics_path());
    const std::string metatiles_after_first = read_file_contents(metatiles_path());

    // Second append with identical inputs must not accumulate anything.
    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6).has_value());
    ASSERT_TRUE(appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2).has_value());

    const std::string graphics_after_second = read_file_contents(graphics_path());
    const std::string metatiles_after_second = read_file_contents(metatiles_path());

    EXPECT_EQ(graphics_after_first, graphics_after_second) << "repeated graphics append is not byte-identical";
    EXPECT_EQ(metatiles_after_first, metatiles_after_second) << "repeated metatiles append is not byte-identical";

    EXPECT_EQ(count_occurrences(graphics_after_second, "gTilesetTiles_PorytilesManaged_General"), 1U)
        << "managed tiles declaration should appear exactly once";
    EXPECT_EQ(count_occurrences(metatiles_after_second, "gMetatiles_PorytilesManaged_General"), 1U)
        << "managed metatiles declaration should appear exactly once";
}

TEST_F(IncbinDeclarationAppenderTest_ExpansionFrlgStock, AppendRelocatesMisplacedDeclarations)
{
    // Recreate the old-bug shape: a managed declaration wrongly parked inside the trailing FRLG block.
    insert_line_before_last_endif(
        graphics_path(), "const u32 gTilesetTiles_PorytilesManaged_General[] = INCBIN_U32(\"stale/tiles.4bpp.lz\");");
    insert_line_before_last_endif(
        metatiles_path(), "const u16 gMetatiles_PorytilesManaged_General[] = INCBIN_U16(\"stale/metatiles.bin\");");

    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6).has_value());
    ASSERT_TRUE(appender_->append_metatiles_declarations("gTileset_General", "data/tilesets/primary", 2).has_value());

    const std::string graphics = read_file_contents(graphics_path());
    const std::string metatiles = read_file_contents(metatiles_path());

    // The stale copy is gone: exactly one occurrence, sitting after the final #endif.
    EXPECT_EQ(count_occurrences(graphics, "gTilesetTiles_PorytilesManaged_General"), 1U)
        << "misplaced graphics declaration was not relocated";
    EXPECT_LT(graphics.rfind("#endif"), graphics.find("gTilesetTiles_PorytilesManaged_General"))
        << "relocated graphics declaration is not after the final #endif";
    EXPECT_EQ(graphics.find("stale/tiles.4bpp.lz"), std::string::npos) << "stale graphics declaration survived";

    EXPECT_EQ(count_occurrences(metatiles, "gMetatiles_PorytilesManaged_General"), 1U)
        << "misplaced metatiles declaration was not relocated";
    EXPECT_LT(metatiles.rfind("#endif"), metatiles.find("gMetatiles_PorytilesManaged_General"))
        << "relocated metatiles declaration is not after the final #endif";
    EXPECT_EQ(metatiles.find("stale/metatiles.bin"), std::string::npos) << "stale metatiles declaration survived";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, RemoveIgnoresSharedShorthandPrefix)
{
    // Two managed tilesets whose shorthands share a prefix: "General" and "GeneralTwo".
    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6).has_value());
    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_GeneralTwo", "data/tilesets/primary", 6).has_value());

    // Removing "General" must not touch "GeneralTwo" (which merely shares the prefix).
    ASSERT_TRUE(appender_->remove_declarations("gTileset_General").has_value());

    const std::string content = read_file_contents(graphics_path());

    // The bracket disambiguates "General[]" from "GeneralTwo[]".
    EXPECT_EQ(content.find("gTilesetTiles_PorytilesManaged_General["), std::string::npos)
        << "gTileset_General tiles declaration was not removed";
    EXPECT_NE(content.find("gTilesetTiles_PorytilesManaged_GeneralTwo"), std::string::npos)
        << "gTileset_GeneralTwo tiles declaration was wrongly removed";
    EXPECT_NE(content.find("gTilesetPalettes_PorytilesManaged_GeneralTwo"), std::string::npos)
        << "gTileset_GeneralTwo palette array header was wrongly removed";
    EXPECT_NE(content.find("data/tilesets/primary/general_two/porytiles_bin/palettes/00.gbapal"), std::string::npos)
        << "gTileset_GeneralTwo palette body was wrongly removed";
}

TEST_F(IncbinDeclarationAppenderTest_VanillaStock, AppendIgnoresSharedShorthandPrefix)
{
    // Append GeneralTwo, then General twice. General's upsert-strip must not disturb GeneralTwo.
    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_GeneralTwo", "data/tilesets/primary", 6).has_value());
    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6).has_value());
    ASSERT_TRUE(appender_->append_graphics_declarations("gTileset_General", "data/tilesets/primary", 6).has_value());

    const std::string content = read_file_contents(graphics_path());

    EXPECT_EQ(count_occurrences(content, "gTilesetTiles_PorytilesManaged_GeneralTwo"), 1U)
        << "gTileset_GeneralTwo declaration was disturbed by gTileset_General upsert";
    EXPECT_EQ(count_occurrences(content, "gTilesetTiles_PorytilesManaged_General["), 1U)
        << "gTileset_General should appear exactly once after repeated append";
}

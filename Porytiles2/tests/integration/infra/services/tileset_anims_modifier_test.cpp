#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/services/project_tileset_anims_modifier.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

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

/**
 * @brief Counts occurrences of a substring in a string.
 */
[[nodiscard]] std::size_t count_occurrences(const std::string &haystack, const std::string &needle)
{
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.length();
    }
    return count;
}

} // namespace

/**
 * @brief Base fixture for TilesetAnimsModifier tests.
 *
 * @details
 * This fixture copies the test project to a temp directory so we can modify files
 * without affecting the original test data.
 */
class TilesetAnimsModifierTestBase : public ::testing::Test {
  protected:
    [[nodiscard]] virtual std::filesystem::path source_project_path() const = 0;

    void SetUp() override
    {
        // Create a unique temp directory for this test
        temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_tileset_anims_modifier_test";
        std::filesystem::remove_all(temp_dir_);
        std::filesystem::create_directories(temp_dir_);

        // Copy the source project to temp
        copy_directory(source_project_path(), temp_dir_);

        ASSERT_TRUE(std::filesystem::exists(temp_dir_)) << "Temp project directory not created at: " << temp_dir_;

        formatter_ = std::make_unique<PlainTextFormatter>();
        diagnostics_ = std::make_unique<BufferedUserDiagnostics>();

        // Setup config with default values (data/tilesets/primary, data/tilesets/secondary)
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<DefaultProvider>());
        config_ = std::make_unique<LazyLayeredConfig>(formatter_.get(), std::move(providers));

        modifier_ = std::make_unique<ProjectTilesetAnimsModifier>(
            temp_dir_, config_.get(), formatter_.get(), diagnostics_.get());
    }

    void TearDown() override
    {
        // Clean up temp directory
        std::filesystem::remove_all(temp_dir_);
    }

    [[nodiscard]] std::filesystem::path tileset_anims_c_path() const
    {
        return temp_dir_ / "src" / "tileset_anims.c";
    }

    [[nodiscard]] std::filesystem::path tileset_anims_h_path() const
    {
        return temp_dir_ / "include" / "tileset_anims.h";
    }

    // Backwards compatibility alias
    [[nodiscard]] std::filesystem::path tileset_anims_path() const
    {
        return tileset_anims_c_path();
    }

    std::filesystem::path temp_dir_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<UserDiagnostics> diagnostics_;
    std::unique_ptr<LazyLayeredConfig> config_;
    std::unique_ptr<ProjectTilesetAnimsModifier> modifier_;
};

/**
 * @brief Tests using the pokeemerald_vanilla_stock mock project.
 */
class TilesetAnimsModifierTest_VanillaStock : public TilesetAnimsModifierTestBase {
  protected:
    [[nodiscard]] std::filesystem::path source_project_path() const override
    {
        return "Resources/Tests/integration/shared/repos/pokeemerald_vanilla_stock";
    }
};

TEST_F(TilesetAnimsModifierTest_VanillaStock, WiresIncludeForPrimaryTileset)
{
    auto result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string content = read_file_contents(tileset_anims_path());

    // Verify the include directive was added
    EXPECT_NE(
        content.find("#include \"porytiles_generated/tilesets/general/generated_anim_code.h\""),
        std::string::npos)
        << "Include directive for gTileset_General not found in tileset_anims.c";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, WiresIncludeForSecondaryTileset)
{
    auto result = modifier_->wire_include_for_tileset("gTileset_Rustboro", /*is_secondary=*/true);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string content = read_file_contents(tileset_anims_path());

    // Verify the include directive was added
    EXPECT_NE(
        content.find("#include \"porytiles_generated/tilesets/rustboro/generated_anim_code.h\""),
        std::string::npos)
        << "Include directive for gTileset_Rustboro not found in tileset_anims.c";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, AppendsAtBottomOfFile)
{
    auto result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string content = read_file_contents(tileset_anims_path());

    // Find positions of various elements
    const std::size_t new_include_pos =
        content.find("#include \"porytiles_generated/tilesets/general/generated_anim_code.h\"");
    const std::size_t porytiles_comment_pos = content.find("// [Porytiles] Auto-generated include. Do not remove.");

    ASSERT_NE(new_include_pos, std::string::npos) << "New include not found";
    ASSERT_NE(porytiles_comment_pos, std::string::npos) << "Porytiles comment not found";

    // Comment should appear before the include
    EXPECT_LT(porytiles_comment_pos, new_include_pos) << "Comment should appear before the include directive";

    // The include should be near the end of the file (after any function definitions)
    // Find last closing brace of a function
    const std::size_t last_closing_brace = content.rfind('}');
    ASSERT_NE(last_closing_brace, std::string::npos) << "No closing brace found in file";

    // The new include should appear after all the function definitions
    EXPECT_GT(new_include_pos, last_closing_brace) << "New include should appear at bottom of file, after functions";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, IsIdempotent_SkipsExistingInclude)
{
    // Wire include first time
    auto result1 = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result1.has_value()) << "First wire_include_for_tileset failed";

    // Wire include second time (should be idempotent)
    auto result2 = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result2.has_value()) << "Second wire_include_for_tileset should succeed (idempotent)";

    const std::string content = read_file_contents(tileset_anims_path());

    // Count occurrences - should only be 1
    const std::size_t count =
        count_occurrences(content, "porytiles_generated/tilesets/general/generated_anim_code.h");
    EXPECT_EQ(count, 1) << "Include directive should only appear once (idempotency)";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, RemovesIncludeAndComment)
{
    // First wire the include
    auto wire_result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(wire_result.has_value()) << "wire_include_for_tileset failed";

    // Verify both include and comment are there
    std::string content = read_file_contents(tileset_anims_path());
    ASSERT_NE(content.find("porytiles_generated/tilesets/general/generated_anim_code.h"), std::string::npos);
    ASSERT_NE(content.find("// [Porytiles] Auto-generated include. Do not remove."), std::string::npos);

    // Now remove it
    auto remove_result = modifier_->remove_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(remove_result.has_value()) << "remove_include_for_tileset failed";

    // Verify both include and comment are gone
    content = read_file_contents(tileset_anims_path());
    EXPECT_EQ(content.find("porytiles_generated/tilesets/general/generated_anim_code.h"), std::string::npos)
        << "Include directive should have been removed";
    EXPECT_EQ(content.find("// [Porytiles] Auto-generated include. Do not remove."), std::string::npos)
        << "Porytiles comment should have been removed";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, PreservesExistingContent)
{
    // Get original content
    const std::string original_content = read_file_contents(tileset_anims_path());
    ASSERT_FALSE(original_content.empty()) << "Original tileset_anims.c is empty";

    // Wire include
    auto result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string new_content = read_file_contents(tileset_anims_path());

    // Verify original includes are still present
    EXPECT_NE(new_content.find("#include \"global.h\""), std::string::npos) << "Original global.h include was removed";
    EXPECT_NE(new_content.find("#include \"fieldmap.h\""), std::string::npos)
        << "Original fieldmap.h include was removed";

    // Verify original function declarations are still present
    EXPECT_NE(new_content.find("InitTilesetAnim_General"), std::string::npos)
        << "Original InitTilesetAnim_General was removed";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, RejectsInvalidTilesetName)
{
    auto result = modifier_->wire_include_for_tileset("InvalidTileset", /*is_secondary=*/false);
    EXPECT_FALSE(result.has_value()) << "Should reject tileset name not starting with gTileset_";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, HandlesSnakeCaseConversion)
{
    // Use a tileset with PascalCase shorthand to test snake_case conversion
    auto result = modifier_->wire_include_for_tileset("gTileset_BattleFrontier", /*is_secondary=*/true);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string content = read_file_contents(tileset_anims_path());

    // Verify the path uses snake_case
    EXPECT_NE(
        content.find("#include \"porytiles_generated/tilesets/battle_frontier/generated_anim_code.h\""),
        std::string::npos)
        << "snake_case path conversion not working correctly";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, RemoveIsIdempotent_SkipsMissingInclude)
{
    // Try to remove an include that doesn't exist
    auto result = modifier_->remove_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    EXPECT_TRUE(result.has_value())
        << "remove_include_for_tileset should succeed (idempotent) even when include missing";

    // Verify file wasn't corrupted
    const std::string content = read_file_contents(tileset_anims_path());
    EXPECT_NE(content.find("#include \"global.h\""), std::string::npos) << "File content was corrupted";
}

// ============================================================================
// Header file declaration tests
// ============================================================================

TEST_F(TilesetAnimsModifierTest_VanillaStock, WiresDeclarationToHeaderFile)
{
    auto result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string h_content = read_file_contents(tileset_anims_h_path());

    // Verify the declaration was added with PorytilesManaged prefix
    EXPECT_NE(h_content.find("void InitTilesetAnim_PorytilesManaged_General(void);"), std::string::npos)
        << "Declaration for gTileset_General not found in tileset_anims.h";

    // Verify the Porytiles comment was added
    EXPECT_NE(h_content.find("// [Porytiles] Auto-generated declaration. Do not remove."), std::string::npos)
        << "Porytiles declaration comment not found in tileset_anims.h";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, DeclarationIsInsertedBeforeEndif)
{
    auto result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string h_content = read_file_contents(tileset_anims_h_path());

    const std::size_t decl_pos = h_content.find("InitTilesetAnim_PorytilesManaged_General");
    const std::size_t endif_pos = h_content.find("#endif");

    ASSERT_NE(decl_pos, std::string::npos) << "Declaration not found";
    ASSERT_NE(endif_pos, std::string::npos) << "#endif not found";

    // Declaration should appear before #endif
    EXPECT_LT(decl_pos, endif_pos) << "Declaration should appear before #endif guard";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, DeclarationPreservesPascalCase)
{
    // BattleFrontier should remain PascalCase in the declaration (not snake_case)
    auto result = modifier_->wire_include_for_tileset("gTileset_BattleFrontier", /*is_secondary=*/true);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string h_content = read_file_contents(tileset_anims_h_path());

    // Declaration should use PascalCase shorthand
    EXPECT_NE(h_content.find("void InitTilesetAnim_PorytilesManaged_BattleFrontier(void);"), std::string::npos)
        << "Declaration should preserve PascalCase shorthand";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, RemovesDeclarationFromHeaderFile)
{
    // First wire the include and declaration
    auto wire_result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(wire_result.has_value()) << "wire_include_for_tileset failed";

    // Verify declaration is there
    std::string h_content = read_file_contents(tileset_anims_h_path());
    ASSERT_NE(h_content.find("InitTilesetAnim_PorytilesManaged_General"), std::string::npos);

    // Now remove it
    auto remove_result = modifier_->remove_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(remove_result.has_value()) << "remove_include_for_tileset failed";

    // Verify declaration is gone
    h_content = read_file_contents(tileset_anims_h_path());
    EXPECT_EQ(h_content.find("InitTilesetAnim_PorytilesManaged_General"), std::string::npos)
        << "Declaration should have been removed from tileset_anims.h";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, HeaderFilePreservesExistingDeclarations)
{
    // Get original content
    const std::string original_h_content = read_file_contents(tileset_anims_h_path());
    ASSERT_FALSE(original_h_content.empty()) << "Original tileset_anims.h is empty";

    // Wire include
    auto result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string new_h_content = read_file_contents(tileset_anims_h_path());

    // Verify original declarations are still present
    EXPECT_NE(new_h_content.find("void InitTilesetAnim_General(void);"), std::string::npos)
        << "Original InitTilesetAnim_General declaration was removed";
    EXPECT_NE(new_h_content.find("void InitTilesetAnimations(void);"), std::string::npos)
        << "Original InitTilesetAnimations declaration was removed";
    EXPECT_NE(new_h_content.find("#ifndef GUARD_TILESET_ANIMS_H"), std::string::npos) << "Header guard was removed";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, DeclarationIdempotency)
{
    // Wire include first time
    auto result1 = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result1.has_value()) << "First wire_include_for_tileset failed";

    // Wire include second time (should be idempotent)
    auto result2 = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result2.has_value()) << "Second wire_include_for_tileset should succeed (idempotent)";

    const std::string h_content = read_file_contents(tileset_anims_h_path());

    // Count occurrences - should only be 1
    const std::size_t count = count_occurrences(h_content, "InitTilesetAnim_PorytilesManaged_General");
    EXPECT_EQ(count, 1) << "Declaration should only appear once (idempotency)";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, DeclarationHasBlankLineBeforeEndif)
{
    // Wire a single declaration
    auto result = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result.has_value()) << "wire_include_for_tileset failed";

    const std::string h_content = read_file_contents(tileset_anims_h_path());

    // There should be a blank line between the declaration and #endif
    // The pattern is: declaration line, blank line, #endif line
    EXPECT_NE(h_content.find("General(void);\n\n#endif"), std::string::npos)
        << "Declaration should have a blank line before #endif guard";
}

TEST_F(TilesetAnimsModifierTest_VanillaStock, MultipleDeclarationsHaveProperSpacing)
{
    // Wire two declarations
    auto result1 = modifier_->wire_include_for_tileset("gTileset_General", /*is_secondary=*/false);
    ASSERT_TRUE(result1.has_value()) << "First wire_include_for_tileset failed";

    auto result2 = modifier_->wire_include_for_tileset("gTileset_Rustboro", /*is_secondary=*/true);
    ASSERT_TRUE(result2.has_value()) << "Second wire_include_for_tileset failed";

    const std::string h_content = read_file_contents(tileset_anims_h_path());

    // Verify both declarations exist
    ASSERT_NE(h_content.find("InitTilesetAnim_PorytilesManaged_General"), std::string::npos);
    ASSERT_NE(h_content.find("InitTilesetAnim_PorytilesManaged_Rustboro"), std::string::npos);

    // Verify proper spacing: single blank line between declarations
    // Pattern: first decl, blank line, comment, second decl
    EXPECT_NE(h_content.find("General(void);\n\n// [Porytiles]"), std::string::npos)
        << "Should have exactly one blank line between declarations";

    // Verify blank line before #endif (after last declaration)
    EXPECT_NE(h_content.find("Rustboro(void);\n\n#endif"), std::string::npos)
        << "Last declaration should have a blank line before #endif guard";

    // Verify no double blank lines between declarations (would indicate a bug)
    EXPECT_EQ(h_content.find("General(void);\n\n\n"), std::string::npos)
        << "Should NOT have double blank lines between declarations";
}

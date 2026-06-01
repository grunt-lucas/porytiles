#include "porytiles/utilities/c_parser/c_parser_facade.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class CParserFacadeTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] std::filesystem::path test_resource_path(const std::string &relative_path) const
    {
        // Tests are run from the build directory, resources are relative to repo root
        std::filesystem::path repo_root = std::filesystem::current_path();
        while (!std::filesystem::exists(repo_root / "resources") && repo_root.has_parent_path()) {
            repo_root = repo_root.parent_path();
        }
        return repo_root / "resources" / relative_path;
    }

    [[nodiscard]] std::filesystem::path create_temp_file(const std::string &content)
    {
        std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "porytiles_driver_test.h";
        std::ofstream file{temp_path};
        file << content;
        file.close();
        temp_files_.push_back(temp_path);
        return temp_path;
    }

    void TearDown() override
    {
        for (const auto &path : temp_files_) {
            std::filesystem::remove(path);
        }
    }

    template <typename T>
    [[nodiscard]] std::string get_all_error_text(const ChainableResult<T> &result)
    {
        std::string error_text;
        for (const auto &err : result.chain()) {
            auto details = err->details(formatter_);
            for (const auto &line : details) {
                error_text += line + "\n";
            }
        }
        return error_text;
    }

  private:
    std::vector<std::filesystem::path> temp_files_;
};

TEST_F(CParserFacadeTests, ParseDefinesFromRealFile)
{
    auto file_path = test_resource_path("Tests/integration/shared/c_parser/metatile_behaviors_define.h");
    CParserFacade driver{file_path, &formatter_};

    auto result = driver.parse_defines();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &defines = result.value();
    // File has many defines, check a few key ones
    EXPECT_GT(defines.size(), 200);

    EXPECT_EQ(defines[0].name(), "GUARD_METATILE_BEHAVIORS_H");
    EXPECT_TRUE(defines[0].is_flag());

    // Find MB_NORMAL (should be 0x00)
    auto it = std::find_if(
        defines.begin(), defines.end(), [](const DefineStatement &def) { return def.name() == "MB_NORMAL"; });
    ASSERT_NE(it, defines.end());
    EXPECT_EQ(it->int_value(), 0);

    // Find MB_DEEP_WATER (should be 0x12)
    it = std::find_if(
        defines.begin(), defines.end(), [](const DefineStatement &def) { return def.name() == "MB_DEEP_WATER"; });
    ASSERT_NE(it, defines.end());
    EXPECT_EQ(it->int_value(), 0x12);

    // Find MB_INVALID (should be 0xFF)
    it = std::find_if(
        defines.begin(), defines.end(), [](const DefineStatement &def) { return def.name() == "MB_INVALID"; });
    ASSERT_NE(it, defines.end());
    EXPECT_EQ(it->int_value(), 0xFF);
}

TEST_F(CParserFacadeTests, ParseEnumsFromRealFile)
{
    auto file_path = test_resource_path("Tests/integration/shared/c_parser/metatile_behaviors_enum.h");
    CParserFacade driver{file_path, &formatter_};

    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value()) << get_all_error_text(enums_result);
    const auto &enums = enums_result.value();
    ASSERT_GE(enums.size(), 1);

    EXPECT_GT(enums[0].members().size(), 0);

    EXPECT_EQ(enums[0].members().at(0).name(), "MB_NORMAL");
    EXPECT_EQ(enums[0].members().at(0).int_value(), 0);

    EXPECT_EQ(enums[0].members().at(38).name(), "MB_THIN_ICE");
    EXPECT_EQ(enums[0].members().at(38).int_value(), 38);

    EXPECT_EQ(enums[0].members().at(143).name(), "MB_QUESTIONNAIRE");
    EXPECT_EQ(enums[0].members().at(143).int_value(), 143);
}

TEST_F(CParserFacadeTests, ParseDefinesAndEnumsFromRealFile)
{
    auto file_path = test_resource_path("Tests/integration/shared/c_parser/metatile_behaviors_mixed.h");
    CParserFacade driver{file_path, &formatter_};

    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value()) << get_all_error_text(defines_result);
    const auto &defines = defines_result.value();
    EXPECT_GT(defines.size(), 0);

    EXPECT_EQ(defines.at(0).name(), "GUARD_METATILE_BEHAVIORS_H");
    EXPECT_TRUE(defines.at(0).is_flag());

    EXPECT_EQ(defines.at(1).name(), "MB_FOO");
    EXPECT_TRUE(defines.at(1).has_int_value());
    EXPECT_EQ(defines.at(1).int_value(), 1024);

    EXPECT_EQ(defines.at(2).name(), "MB_BAR");
    EXPECT_TRUE(defines.at(2).has_int_value());
    EXPECT_EQ(defines.at(2).int_value(), 1025);

    EXPECT_EQ(defines.at(3).name(), "MB_BAZ");
    EXPECT_TRUE(defines.at(3).has_int_value());
    EXPECT_EQ(defines.at(3).int_value(), 1026);

    EXPECT_EQ(defines.at(4).name(), "MB_BAT");
    EXPECT_TRUE(defines.at(4).has_int_value());
    EXPECT_EQ(defines.at(4).int_value(), 2048);

    EXPECT_EQ(defines.at(5).name(), "SOME_OTHER_DEFINE");
    EXPECT_TRUE(defines.at(5).has_string_value());
    EXPECT_EQ(defines.at(5).string_value(), "hello world");

    // last file define is MB_INVALID
    EXPECT_EQ(defines.at(defines.size() - 1).name(), "MB_INVALID");
    EXPECT_TRUE(defines.at(defines.size() - 1).has_int_value());
    EXPECT_EQ(defines.at(defines.size() - 1).int_value(), 255);

    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value()) << get_all_error_text(enums_result);
    const auto &enums = enums_result.value();
    ASSERT_GE(enums.size(), 1);

    EXPECT_GT(enums[0].members().size(), 0);

    EXPECT_EQ(enums[0].members().at(0).name(), "MB_NORMAL");
    EXPECT_EQ(enums[0].members().at(0).int_value(), 0);

    EXPECT_EQ(enums[0].members().at(38).name(), "MB_THIN_ICE");
    EXPECT_EQ(enums[0].members().at(38).int_value(), 38);

    EXPECT_EQ(enums[0].members().at(143).name(), "MB_QUESTIONNAIRE");
    EXPECT_EQ(enums[0].members().at(143).int_value(), 143);
}

TEST_F(CParserFacadeTests, ParseDefinesFromTempFile)
{
    auto temp_path = create_temp_file("#define FOO 123\n#define BAR 0xFF\n#define BAZ (1 << 4)");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_defines();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &defines = result.value();
    ASSERT_EQ(defines.size(), 3);
    EXPECT_EQ(defines[0].name(), "FOO");
    EXPECT_EQ(defines[0].int_value(), 123);
    EXPECT_EQ(defines[1].name(), "BAR");
    EXPECT_EQ(defines[1].int_value(), 255);
    EXPECT_EQ(defines[2].name(), "BAZ");
    EXPECT_EQ(defines[2].int_value(), 16);
}

TEST_F(CParserFacadeTests, ParseEnumsFromTempFile)
{
    auto temp_path = create_temp_file("enum { A, B = 10, C };");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_enums();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &enums = result.value();
    ASSERT_EQ(enums.size(), 1);

    const auto &members = enums[0].members();
    ASSERT_EQ(members.size(), 3);
    EXPECT_EQ(members[0].name(), "A");
    EXPECT_EQ(members[0].int_value(), 0);
    EXPECT_EQ(members[1].name(), "B");
    EXPECT_EQ(members[1].int_value(), 10);
    EXPECT_EQ(members[2].name(), "C");
    EXPECT_EQ(members[2].int_value(), 11);
}

TEST_F(CParserFacadeTests, NonExistentFileReturnsFileNotFoundError)
{
    CParserFacade driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_defines();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

TEST_F(CParserFacadeTests, NonExistentFileReturnsFileNotFoundErrorForEnums)
{
    CParserFacade driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_enums();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

// NOTE: Testing the "failed to load file" error (for files that exist but can't be opened)
// would require creating a file with no read permissions, which is platform-specific
// and fragile for unit tests. The code path is simple and covered by manual testing.

TEST_F(CParserFacadeTests, LexerErrorIncludesFileContext)
{
    auto temp_path = create_temp_file("#define FOO \"unterminated string");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_defines();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("porytiles_driver_test.h"), std::string::npos);
    EXPECT_NE(error_text.find("unterminated string"), std::string::npos);
}

TEST_F(CParserFacadeTests, ParserErrorIncludesFileContext)
{
    auto temp_path = create_temp_file("#define FOO UNKNOWN_IDENTIFIER");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_defines();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("porytiles_driver_test.h"), std::string::npos);
    EXPECT_NE(error_text.find("unknown identifier"), std::string::npos);
}

TEST_F(CParserFacadeTests, EnumParseErrorIncludesFileContext)
{
    auto temp_path = create_temp_file("enum FOO;"); // Missing opening brace
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_enums();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("porytiles_driver_test.h"), std::string::npos);
    EXPECT_NE(error_text.find("expected '{' after 'enum'"), std::string::npos);
}

TEST_F(CParserFacadeTests, MultipleParseCallsReuseLoadedFile)
{
    auto temp_path = create_temp_file("#define A 1\n#define B 2\nenum { X, Y };");
    CParserFacade driver{temp_path, &formatter_};

    // First call loads file
    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value());
    EXPECT_EQ(defines_result.value().size(), 2);

    // Second call should still work (file remains loaded)
    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value());
    EXPECT_EQ(enums_result.value().size(), 1);
}

TEST_F(CParserFacadeTests, ParseDefinesDoesNotResetParserState)
{
    // Calling parse_defines twice should give the same result
    auto temp_path = create_temp_file("#define FOO 123\n#define BAR 456");
    CParserFacade driver{temp_path, &formatter_};

    auto result1 = driver.parse_defines();
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().size(), 2);

    auto result2 = driver.parse_defines();
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().size(), 2);
}

TEST_F(CParserFacadeTests, EmptyFileReturnsEmptyResults)
{
    auto temp_path = create_temp_file("");
    CParserFacade driver{temp_path, &formatter_};

    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value());
    EXPECT_TRUE(defines_result.value().empty());

    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value());
    EXPECT_TRUE(enums_result.value().empty());
}

TEST_F(CParserFacadeTests, FileWithOnlyCommentsReturnsEmptyResults)
{
    auto temp_path = create_temp_file("// This is a comment\n/* Block comment */\n");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_defines();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(CParserFacadeTests, MixedDefinesAndEnums)
{
    auto temp_path = create_temp_file(R"(
#define FOO 1
enum { A, B };
#define BAR 2
enum { C, D };
)");
    CParserFacade driver{temp_path, &formatter_};

    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value());
    EXPECT_EQ(defines_result.value().size(), 2);

    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value());
    EXPECT_EQ(enums_result.value().size(), 2);
}

TEST_F(CParserFacadeTests, ParsePointerArraysFromGeneratedHeader)
{
    auto file_path = test_resource_path("Tests/integration/shared/c_parser/generated_anim_code.h");
    CParserFacade driver{file_path, &formatter_};

    auto result = driver.parse_pointer_arrays();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &arrays = result.value();
    EXPECT_EQ(arrays.size(), 5);

    // Find Flower array
    auto flower_it = std::find_if(arrays.begin(), arrays.end(), [](const ArrayDeclaration &arr) {
        return arr.name() == "gTilesetAnims_PorytilesManaged_General_Flower";
    });
    ASSERT_NE(flower_it, arrays.end());
    EXPECT_EQ(flower_it->elements().size(), 4); // Frame0, Frame1, Frame0, Frame2

    // Find Water array
    auto water_it = std::find_if(arrays.begin(), arrays.end(), [](const ArrayDeclaration &arr) {
        return arr.name() == "gTilesetAnims_PorytilesManaged_General_Water";
    });
    ASSERT_NE(water_it, arrays.end());
    EXPECT_EQ(water_it->elements().size(), 8);
}

TEST_F(CParserFacadeTests, ParsePointerArraysFromTempFile)
{
    auto temp_path = create_temp_file(R"(
const u16 *const myArray[] = {
    myArray_Frame0,
    myArray_Frame1,
    myArray_Frame2
};
)");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_pointer_arrays();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &arrays = result.value();
    ASSERT_EQ(arrays.size(), 1);
    EXPECT_EQ(arrays[0].name(), "myArray");
    EXPECT_EQ(arrays[0].elements().size(), 3);
    EXPECT_EQ(arrays[0].elements()[0], "myArray_Frame0");
    EXPECT_EQ(arrays[0].elements()[1], "myArray_Frame1");
    EXPECT_EQ(arrays[0].elements()[2], "myArray_Frame2");
}

TEST_F(CParserFacadeTests, ParsePointerArraysNonExistentFileReturnsError)
{
    CParserFacade driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_pointer_arrays();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

TEST_F(CParserFacadeTests, ParseFunctionsFromGeneratedHeader)
{
    auto file_path = test_resource_path("Tests/integration/shared/c_parser/generated_anim_code.h");
    CParserFacade driver{file_path, &formatter_};

    auto result = driver.parse_functions();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &functions = result.value();
    // Should have at least 5 functions: 2 QueueAnimTiles, 1 driver, 1 Init, possibly more
    EXPECT_GE(functions.size(), 4);

    // Find QueueAnimTiles_PorytilesManaged_General_Flower
    auto flower_it = std::find_if(functions.begin(), functions.end(), [](const FunctionDefinition &func) {
        return func.name() == "QueueAnimTiles_PorytilesManaged_General_Flower";
    });
    ASSERT_NE(flower_it, functions.end());
    EXPECT_FALSE(flower_it->body_tokens().empty());
}

TEST_F(CParserFacadeTests, ParseFunctionsWithPrefixFilter)
{
    auto file_path = test_resource_path("Tests/integration/shared/c_parser/generated_anim_code.h");
    CParserFacade driver{file_path, &formatter_};

    auto result = driver.parse_functions("QueueAnimTiles_");
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &functions = result.value();
    EXPECT_EQ(functions.size(), 5);

    for (const auto &func : functions) {
        EXPECT_TRUE(func.name().starts_with("QueueAnimTiles_"));
    }
}

TEST_F(CParserFacadeTests, ParseFunctionsFromTempFile)
{
    auto temp_path = create_temp_file(R"(
static void funcA(int x) {
    int y = x + 1;
}

void funcB(void) {
    funcA(5);
}
)");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_functions();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &functions = result.value();
    ASSERT_EQ(functions.size(), 2);
    EXPECT_EQ(functions[0].name(), "funcA");
    EXPECT_EQ(functions[1].name(), "funcB");
}

TEST_F(CParserFacadeTests, ParseFunctionsNonExistentFileReturnsError)
{
    CParserFacade driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_functions();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

TEST_F(CParserFacadeTests, ParseFunctionBodyContainsExpectedTokens)
{
    auto file_path = test_resource_path("Tests/integration/shared/c_parser/generated_anim_code.h");
    CParserFacade driver{file_path, &formatter_};

    auto result = driver.parse_functions("QueueAnimTiles_PorytilesManaged_General_Flower");
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &functions = result.value();
    ASSERT_EQ(functions.size(), 1);

    const auto &body = functions[0].body_tokens();

    bool found_tile_offset = false;
    bool found_508 = false;
    for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i].is(TokenType::identifier) && body[i].text() == "TILE_OFFSET_4BPP") {
            found_tile_offset = true;
            // Next token after ( should be 12
            if (i + 2 < body.size() && body[i + 1].is(TokenType::left_paren) &&
                body[i + 2].is(TokenType::integer_literal) && body[i + 2].int_value() == 508) {
                found_508 = true;
            }
        }
    }
    EXPECT_TRUE(found_tile_offset) << "Expected to find TILE_OFFSET_4BPP in function body";
    EXPECT_TRUE(found_508) << "Expected to find TILE_OFFSET_4BPP(508) in function body";
}

TEST_F(CParserFacadeTests, ParseIncbinArraysSinglePathDeclarations)
{
    auto temp_path = create_temp_file(R"(
static const u16 sTilesetAnims_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");
static const u16 sTilesetAnims_General_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/1.4bpp");
)");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_incbin_arrays();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &incbins = result.value();
    ASSERT_EQ(incbins.size(), 2);
    EXPECT_EQ(incbins[0].variable_name(), "sTilesetAnims_General_Flower_Frame0");
    EXPECT_EQ(incbins[0].macro_name(), "INCBIN_U16");
    ASSERT_EQ(incbins[0].paths().size(), 1);
    EXPECT_EQ(incbins[0].paths()[0], "data/tilesets/primary/general/anim/flower/0.4bpp");
}

TEST_F(CParserFacadeTests, ParseIncbinArraysWithPrefixFilter)
{
    auto temp_path = create_temp_file(R"(
static const u16 sTilesetAnims_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");
static const u16 sTilesetAnims_General_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/1.4bpp");
const u16 gTilesetAnims_BattleFrontier_Flag_Frame0[] = INCBIN_U16("data/tilesets/secondary/battle_frontier/anim/flag/0.4bpp");
)");
    CParserFacade driver{temp_path, &formatter_};

    auto s_result = driver.parse_incbin_arrays("sTilesetAnims_General_");
    ASSERT_TRUE(s_result.has_value()) << get_all_error_text(s_result);
    EXPECT_EQ(s_result.value().size(), 2);

    auto g_result = driver.parse_incbin_arrays("gTilesetAnims_BattleFrontier_");
    ASSERT_TRUE(g_result.has_value()) << get_all_error_text(g_result);
    EXPECT_EQ(g_result.value().size(), 1);
}

TEST_F(CParserFacadeTests, ParseIncbinArraysMixedPrefixesSameTilesetName)
{
    // Reproduces the pokefirered-expansion scenario where BattleFrontier reuses
    // gTilesetAnims_General_* names for shared animations, while the actual
    // General tileset uses sTilesetAnims_General_* declarations.
    auto temp_path = create_temp_file(R"(
// Actual General tileset animations (s-prefix)
static const u16 sTilesetAnims_General_Flower_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/0.4bpp");
static const u16 sTilesetAnims_General_Flower_Frame1[] = INCBIN_U16("data/tilesets/primary/general/anim/flower/1.4bpp");
static const u16 sTilesetAnims_General_Water_Frame0[] = INCBIN_U16("data/tilesets/primary/general/anim/water/0.4bpp");

// BattleFrontier shared animations reusing General_* naming (g-prefix)
const u16 gTilesetAnims_General_Water_Frame0[] = INCBIN_U16("data/tilesets/primary/battle_frontier_outside/anim/water/0.4bpp");
const u16 gTilesetAnims_General_Water_Frame1[] = INCBIN_U16("data/tilesets/primary/battle_frontier_outside/anim/water/1.4bpp");
)");
    CParserFacade driver{temp_path, &formatter_};

    // g-prefix filter for "General_" should match only the BattleFrontier reused names
    auto g_result = driver.parse_incbin_arrays("gTilesetAnims_General_");
    ASSERT_TRUE(g_result.has_value()) << get_all_error_text(g_result);
    const auto &g_incbins = g_result.value();
    EXPECT_EQ(g_incbins.size(), 2);
    // These point to battle_frontier_outside, not general
    EXPECT_EQ(g_incbins[0].variable_name(), "gTilesetAnims_General_Water_Frame0");
    EXPECT_NE(g_incbins[0].paths()[0].find("battle_frontier_outside"), std::string::npos);

    // s-prefix filter for "General_" should match the actual General tileset animations
    auto s_result = driver.parse_incbin_arrays("sTilesetAnims_General_");
    ASSERT_TRUE(s_result.has_value()) << get_all_error_text(s_result);
    const auto &s_incbins = s_result.value();
    EXPECT_EQ(s_incbins.size(), 3);
    // These point to actual general tileset data
    EXPECT_EQ(s_incbins[0].variable_name(), "sTilesetAnims_General_Flower_Frame0");
    EXPECT_NE(s_incbins[0].paths()[0].find("primary/general"), std::string::npos);
}

TEST_F(CParserFacadeTests, ParseIncbinArraysMultiDimensional)
{
    auto temp_path = create_temp_file(R"(
const u16 gTilesetPalettes_General[][16] = {
    INCBIN_U16("data/tilesets/primary/general/palettes/00.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/01.gbapal"),
};
)");
    CParserFacade driver{temp_path, &formatter_};

    auto result = driver.parse_incbin_arrays();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &incbins = result.value();
    ASSERT_EQ(incbins.size(), 1);
    EXPECT_EQ(incbins[0].variable_name(), "gTilesetPalettes_General");
    EXPECT_EQ(incbins[0].paths().size(), 2);
}

} // namespace
} // namespace porytiles

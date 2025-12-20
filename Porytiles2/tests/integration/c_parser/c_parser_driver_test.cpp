#include "porytiles2/utilities/c_parser/c_parser_driver.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "porytiles2/utilities/text/plain_text_formatter.hpp"

namespace porytiles2 {
namespace {

class CParserDriverTests : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    // Helper to get path to test resources
    [[nodiscard]] std::filesystem::path test_resource_path(const std::string &relative_path) const
    {
        // Tests are run from the build directory, resources are relative to repo root
        std::filesystem::path repo_root = std::filesystem::current_path();
        while (!std::filesystem::exists(repo_root / "Resources") && repo_root.has_parent_path()) {
            repo_root = repo_root.parent_path();
        }
        return repo_root / "Resources" / relative_path;
    }

    // Helper to create a temporary file with content
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

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(CParserDriverTests, ParseDefinesFromRealFile)
{
    auto file_path = test_resource_path("Tests/integration/c_parser/metatile_behaviors_define.h");
    CParserDriver driver{file_path, &formatter_};

    auto result = driver.parse_defines();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &defines = result.value();
    // File has many defines, check a few key ones
    EXPECT_GT(defines.size(), 200);

    // Check first define
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

TEST_F(CParserDriverTests, ParseEnumsFromRealFile)
{
    auto file_path = test_resource_path("Tests/integration/c_parser/metatile_behaviors_enum.h");
    CParserDriver driver{file_path, &formatter_};

    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value()) << get_all_error_text(enums_result);
    const auto &enums = enums_result.value();
    ASSERT_GE(enums.size(), 1);

    // Check that we have enum members
    EXPECT_GT(enums[0].members().size(), 0);

    // Check some member values
    EXPECT_EQ(enums[0].members().at(0).name(), "MB_NORMAL");
    EXPECT_EQ(enums[0].members().at(0).int_value(), 0);

    EXPECT_EQ(enums[0].members().at(38).name(), "MB_THIN_ICE");
    EXPECT_EQ(enums[0].members().at(38).int_value(), 38);

    EXPECT_EQ(enums[0].members().at(143).name(), "MB_QUESTIONNAIRE");
    EXPECT_EQ(enums[0].members().at(143).int_value(), 143);
}

TEST_F(CParserDriverTests, ParseDefinesAndEnumsFromRealFile)
{
    auto file_path = test_resource_path("Tests/integration/c_parser/metatile_behaviors_mixed.h");
    CParserDriver driver{file_path, &formatter_};

    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value()) << get_all_error_text(defines_result);
    const auto &defines = defines_result.value();
    EXPECT_GT(defines.size(), 0);

    // Check first few file defines
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

    // Check that we have enum members
    EXPECT_GT(enums[0].members().size(), 0);

    // Check some member values
    EXPECT_EQ(enums[0].members().at(0).name(), "MB_NORMAL");
    EXPECT_EQ(enums[0].members().at(0).int_value(), 0);

    EXPECT_EQ(enums[0].members().at(38).name(), "MB_THIN_ICE");
    EXPECT_EQ(enums[0].members().at(38).int_value(), 38);

    EXPECT_EQ(enums[0].members().at(143).name(), "MB_QUESTIONNAIRE");
    EXPECT_EQ(enums[0].members().at(143).int_value(), 143);
}

TEST_F(CParserDriverTests, ParseDefinesFromTempFile)
{
    auto temp_path = create_temp_file("#define FOO 123\n#define BAR 0xFF\n#define BAZ (1 << 4)");
    CParserDriver driver{temp_path, &formatter_};

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

TEST_F(CParserDriverTests, ParseEnumsFromTempFile)
{
    auto temp_path = create_temp_file("enum { A, B = 10, C };");
    CParserDriver driver{temp_path, &formatter_};

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

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(CParserDriverTests, NonExistentFileReturnsFileNotFoundError)
{
    CParserDriver driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_defines();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

TEST_F(CParserDriverTests, NonExistentFileReturnsFileNotFoundErrorForEnums)
{
    CParserDriver driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_enums();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

// NOTE: Testing the "failed to load file" error (for files that exist but can't be opened)
// would require creating a file with no read permissions, which is platform-specific
// and fragile for unit tests. The code path is simple and covered by manual testing.

TEST_F(CParserDriverTests, LexerErrorIncludesFileContext)
{
    auto temp_path = create_temp_file("#define FOO \"unterminated string");
    CParserDriver driver{temp_path, &formatter_};

    auto result = driver.parse_defines();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    // Should include file path in error
    EXPECT_NE(error_text.find("porytiles_driver_test.h"), std::string::npos);
    // Should include the error message
    EXPECT_NE(error_text.find("unterminated string"), std::string::npos);
}

TEST_F(CParserDriverTests, ParserErrorIncludesFileContext)
{
    auto temp_path = create_temp_file("#define FOO UNKNOWN_IDENTIFIER");
    CParserDriver driver{temp_path, &formatter_};

    auto result = driver.parse_defines();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    // Should include file path in error
    EXPECT_NE(error_text.find("porytiles_driver_test.h"), std::string::npos);
    // Should include the error message
    EXPECT_NE(error_text.find("unknown identifier"), std::string::npos);
}

TEST_F(CParserDriverTests, EnumParseErrorIncludesFileContext)
{
    auto temp_path = create_temp_file("enum FOO;"); // Missing opening brace
    CParserDriver driver{temp_path, &formatter_};

    auto result = driver.parse_enums();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    // Should include file path in error
    EXPECT_NE(error_text.find("porytiles_driver_test.h"), std::string::npos);
    // Should include the error message
    EXPECT_NE(error_text.find("expected '{' after 'enum'"), std::string::npos);
}

// ============================================================================
// Caching/Reuse Tests
// ============================================================================

TEST_F(CParserDriverTests, MultipleParseCallsReuseLoadedFile)
{
    auto temp_path = create_temp_file("#define A 1\n#define B 2\nenum { X, Y };");
    CParserDriver driver{temp_path, &formatter_};

    // First call loads file
    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value());
    EXPECT_EQ(defines_result.value().size(), 2);

    // Second call should still work (file remains loaded)
    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value());
    EXPECT_EQ(enums_result.value().size(), 1);
}

TEST_F(CParserDriverTests, ParseDefinesDoesNotResetParserState)
{
    // Calling parse_defines twice should give the same result
    auto temp_path = create_temp_file("#define FOO 123\n#define BAR 456");
    CParserDriver driver{temp_path, &formatter_};

    auto result1 = driver.parse_defines();
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value().size(), 2);

    auto result2 = driver.parse_defines();
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value().size(), 2);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(CParserDriverTests, EmptyFileReturnsEmptyResults)
{
    auto temp_path = create_temp_file("");
    CParserDriver driver{temp_path, &formatter_};

    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value());
    EXPECT_TRUE(defines_result.value().empty());

    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value());
    EXPECT_TRUE(enums_result.value().empty());
}

TEST_F(CParserDriverTests, FileWithOnlyCommentsReturnsEmptyResults)
{
    auto temp_path = create_temp_file("// This is a comment\n/* Block comment */\n");
    CParserDriver driver{temp_path, &formatter_};

    auto result = driver.parse_defines();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(CParserDriverTests, MixedDefinesAndEnums)
{
    auto temp_path = create_temp_file(R"(
#define FOO 1
enum { A, B };
#define BAR 2
enum { C, D };
)");
    CParserDriver driver{temp_path, &formatter_};

    auto defines_result = driver.parse_defines();
    ASSERT_TRUE(defines_result.has_value());
    EXPECT_EQ(defines_result.value().size(), 2);

    auto enums_result = driver.parse_enums();
    ASSERT_TRUE(enums_result.has_value());
    EXPECT_EQ(enums_result.value().size(), 2);
}

// ============================================================================
// Pointer Array Parsing Tests
// ============================================================================

TEST_F(CParserDriverTests, ParsePointerArraysFromGeneratedHeader)
{
    auto file_path = test_resource_path("Tests/integration/c_parser/generated_anim_code.h");
    CParserDriver driver{file_path, &formatter_};

    auto result = driver.parse_pointer_arrays();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &arrays = result.value();
    // Should have 2 frame pointer arrays: Flower and Water
    EXPECT_EQ(arrays.size(), 2);

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
    EXPECT_EQ(water_it->elements().size(), 5); // Frame0, Frame1, Frame2, Frame3, Frame4
}

TEST_F(CParserDriverTests, ParsePointerArraysFromTempFile)
{
    auto temp_path = create_temp_file(R"(
const u16 *const myArray[] = {
    myArray_Frame0,
    myArray_Frame1,
    myArray_Frame2
};
)");
    CParserDriver driver{temp_path, &formatter_};

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

TEST_F(CParserDriverTests, ParsePointerArraysNonExistentFileReturnsError)
{
    CParserDriver driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_pointer_arrays();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

// ============================================================================
// Function Parsing Tests
// ============================================================================

TEST_F(CParserDriverTests, ParseFunctionsFromGeneratedHeader)
{
    auto file_path = test_resource_path("Tests/integration/c_parser/generated_anim_code.h");
    CParserDriver driver{file_path, &formatter_};

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

TEST_F(CParserDriverTests, ParseFunctionsWithPrefixFilter)
{
    auto file_path = test_resource_path("Tests/integration/c_parser/generated_anim_code.h");
    CParserDriver driver{file_path, &formatter_};

    auto result = driver.parse_functions("QueueAnimTiles_");
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &functions = result.value();
    // Should have exactly 2 QueueAnimTiles functions
    EXPECT_EQ(functions.size(), 2);

    // All should have QueueAnimTiles_ prefix
    for (const auto &func : functions) {
        EXPECT_TRUE(func.name().starts_with("QueueAnimTiles_"));
    }
}

TEST_F(CParserDriverTests, ParseFunctionsFromTempFile)
{
    auto temp_path = create_temp_file(R"(
static void funcA(int x) {
    int y = x + 1;
}

void funcB(void) {
    funcA(5);
}
)");
    CParserDriver driver{temp_path, &formatter_};

    auto result = driver.parse_functions();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &functions = result.value();
    ASSERT_EQ(functions.size(), 2);
    EXPECT_EQ(functions[0].name(), "funcA");
    EXPECT_EQ(functions[1].name(), "funcB");
}

TEST_F(CParserDriverTests, ParseFunctionsNonExistentFileReturnsError)
{
    CParserDriver driver{"/nonexistent/path/to/file.h", &formatter_};

    auto result = driver.parse_functions();
    EXPECT_FALSE(result.has_value());

    std::string error_text = get_all_error_text(result);
    EXPECT_NE(error_text.find("file not found"), std::string::npos);
}

TEST_F(CParserDriverTests, ParseFunctionBodyContainsExpectedTokens)
{
    auto file_path = test_resource_path("Tests/integration/c_parser/generated_anim_code.h");
    CParserDriver driver{file_path, &formatter_};

    auto result = driver.parse_functions("QueueAnimTiles_PorytilesManaged_General_Flower");
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &functions = result.value();
    ASSERT_EQ(functions.size(), 1);

    const auto &body = functions[0].body_tokens();

    // Should contain TILE_OFFSET_4BPP identifier
    bool found_tile_offset = false;
    bool found_12 = false;
    for (std::size_t i = 0; i < body.size(); ++i) {
        if (body[i].is(TokenType::identifier) && body[i].text() == "TILE_OFFSET_4BPP") {
            found_tile_offset = true;
            // Next token after ( should be 12
            if (i + 2 < body.size() && body[i + 1].is(TokenType::left_paren) &&
                body[i + 2].is(TokenType::integer_literal) && body[i + 2].int_value() == 12) {
                found_12 = true;
            }
        }
    }
    EXPECT_TRUE(found_tile_offset) << "Expected to find TILE_OFFSET_4BPP in function body";
    EXPECT_TRUE(found_12) << "Expected to find TILE_OFFSET_4BPP(12) in function body";
}

} // namespace
} // namespace porytiles2

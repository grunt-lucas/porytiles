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
    auto file_path = test_resource_path("Tests/integration/services/metatile_behaviors_define.h");
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
    auto file_path = test_resource_path("Tests/integration/services/metatile_behaviors_enum.h");
    CParserDriver driver{file_path, &formatter_};

    auto result = driver.parse_enums();
    ASSERT_TRUE(result.has_value()) << get_all_error_text(result);

    const auto &enums = result.value();
    ASSERT_GE(enums.size(), 1);

    // Check that we have enum members
    EXPECT_GT(enums[0].members().size(), 0);
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

} // namespace
} // namespace porytiles2

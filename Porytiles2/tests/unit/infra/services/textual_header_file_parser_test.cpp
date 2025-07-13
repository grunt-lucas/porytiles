#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "fmt/format.h"

#include "../../../../include/porytiles2/infra/services/src_edit/textual_header_file_parser.hpp"

using namespace porytiles2;

class TextualHeaderFileParserTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a temporary directory for testing
        temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_parser_test";
        std::filesystem::create_directories(temp_dir_);

        parser_ = std::make_unique<TextualHeaderFileParser>();
    }

    void TearDown() override {
        // Clean up temporary directory
        std::filesystem::remove_all(temp_dir_);
    }

    void create_test_file(const std::string &filename, const std::string &content) {
        std::ofstream file{temp_dir_ / filename};
        file << content;
    }

    std::filesystem::path temp_dir_;
    std::unique_ptr<TextualHeaderFileParser> parser_;
};

TEST_F(TextualHeaderFileParserTest, ParseHeaderFileShouldWork) {
    const std::string content = "#include <stdio.h>\n"
                                "\n"
                                "const int gTestValue = 42;\n"
                                "extern void TestFunction();\n";

    create_test_file("test.h", content);

    auto result = parser_->parse_header_file(temp_dir_ / "test.h");

    EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

    const auto &lines = result.value();
    EXPECT_EQ(lines.size(), 4);
    EXPECT_EQ(lines[0], "#include <stdio.h>");
    EXPECT_EQ(lines[1], "");
    EXPECT_EQ(lines[2], "const int gTestValue = 42;");
    EXPECT_EQ(lines[3], "extern void TestFunction();");
}

TEST_F(TextualHeaderFileParserTest, ParseNonExistentFileShouldFail) {
    auto result = parser_->parse_header_file(temp_dir_ / "nonexistent.h");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("Cannot open file for reading") != std::string::npos);
}

TEST_F(TextualHeaderFileParserTest, ParseEmptyFileShouldWork) {
    create_test_file("empty.h", "");

    auto result = parser_->parse_header_file(temp_dir_ / "empty.h");

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(TextualHeaderFileParserTest, ContainsDeclarationShouldFindExisting) {
    const std::string content = "#include <stdio.h>\n"
                                "const u16 gTilesetPalettes_MyTileset[][16] = {\n"
                                "    // palette data\n"
                                "};\n";

    create_test_file("test.h", content);

    auto result = parser_->contains_declaration(temp_dir_ / "test.h", "gTilesetPalettes_MyTileset");

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

TEST_F(TextualHeaderFileParserTest, ContainsDeclarationShouldNotFindMissing) {
    const std::string content = "#include <stdio.h>\n"
                                "const int gTestValue = 42;\n";

    create_test_file("test.h", content);

    auto result = parser_->contains_declaration(temp_dir_ / "test.h", "gTilesetPalettes_MyTileset");

    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.value());
}

TEST_F(TextualHeaderFileParserTest, ContainsDeclarationOnNonExistentFileShouldFail) {
    auto result = parser_->contains_declaration(temp_dir_ / "nonexistent.h", "pattern");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("Cannot open file for reading") != std::string::npos);
}

TEST_F(TextualHeaderFileParserTest, FindInsertionPointShouldReturnEndOfFile) {
    const std::string content = "#include <stdio.h>\n"
                                "const int gTestValue = 42;\n"
                                "extern void TestFunction();\n";

    create_test_file("test.h", content);

    auto result = parser_->find_insertion_point(temp_dir_ / "test.h");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 3); // End of file (3 lines)
}

TEST_F(TextualHeaderFileParserTest, FindInsertionPointOnEmptyFileShouldReturnZero) {
    create_test_file("empty.h", "");

    auto result = parser_->find_insertion_point(temp_dir_ / "empty.h");

    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TextualHeaderFileParserTest, ValidateHeaderStructureShouldPassForValidHeader) {
    const std::string content = "#ifndef TEST_H\n"
                                "#define TEST_H\n"
                                "\n"
                                "#include <stdio.h>\n"
                                "\n"
                                "const u16 gTestData[] = INCBIN_U16(\"path/to/data.bin\");\n"
                                "extern void TestFunction();\n"
                                "\n"
                                "#endif // TEST_H\n";

    create_test_file("valid.h", content);

    auto result = parser_->validate_header_structure(temp_dir_ / "valid.h");

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value());
}

TEST_F(TextualHeaderFileParserTest, ValidateHeaderStructureShouldFailForEmptyFile) {
    create_test_file("empty.h", "");

    auto result = parser_->validate_header_structure(temp_dir_ / "empty.h");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("File is empty") != std::string::npos);
}

TEST_F(TextualHeaderFileParserTest, ValidateHeaderStructureShouldFailForNonCContent) {
    const std::string content = "This is not a C header file.\n"
                                "It just contains random text.\n"
                                "No C patterns here.\n";

    create_test_file("notc.h", content);

    auto result = parser_->validate_header_structure(temp_dir_ / "notc.h");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("does not appear to contain C header content") != std::string::npos);
}

TEST_F(TextualHeaderFileParserTest, ValidateHeaderStructureShouldFailForNonExistentFile) {
    auto result = parser_->validate_header_structure(temp_dir_ / "nonexistent.h");

    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().find("File does not exist") != std::string::npos);
}

TEST_F(TextualHeaderFileParserTest, ValidateHeaderStructureShouldPassForVariousPatterns) {
    // Test different valid C header patterns
    const std::vector<std::string> valid_patterns = {"#include <stdio.h>\n",
                                                     "#ifndef HEADER_H\n#define HEADER_H\n",
                                                     "const int value = 42;\n",
                                                     "extern void func();\n",
                                                     "struct MyStruct {\n    int field;\n};\n",
                                                     "const u16 data[] = INCBIN_U16(\"file.bin\");\n"};

    for (size_t i = 0; i < valid_patterns.size(); ++i) {
        const std::string filename = fmt::format("pattern{}.h", i);
        create_test_file(filename, valid_patterns[i]);

        auto result = parser_->validate_header_structure(temp_dir_ / filename);

        EXPECT_TRUE(result.has_value()) << "Pattern " << i << " should be valid";
        EXPECT_TRUE(result.value()) << "Pattern " << i << " should be valid";
    }
}

TEST_F(TextualHeaderFileParserTest, ContainsDeclarationShouldHandlePartialMatches) {
    const std::string content = "const u16 gTilesetPalettes_MyTileset[][16] = {\n"
                                "    // This contains gTilesetPalettes_OtherTileset in a comment\n"
                                "    INCBIN_U16(\"data/path.bin\"),\n"
                                "};\n";

    create_test_file("test.h", content);

    // Should find the exact match
    auto result1 = parser_->contains_declaration(temp_dir_ / "test.h", "gTilesetPalettes_MyTileset");
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result1.value());

    // Should find the partial match in comment
    auto result2 = parser_->contains_declaration(temp_dir_ / "test.h", "gTilesetPalettes_OtherTileset");
    EXPECT_TRUE(result2.has_value());
    EXPECT_TRUE(result2.value());

    // Should not find non-existent pattern
    auto result3 = parser_->contains_declaration(temp_dir_ / "test.h", "gTilesetPalettes_NonExistent");
    EXPECT_TRUE(result3.has_value());
    EXPECT_FALSE(result3.value());
}
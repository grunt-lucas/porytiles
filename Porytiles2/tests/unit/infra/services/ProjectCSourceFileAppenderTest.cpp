#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "porytiles2/infra/project/ProjectPaths.hpp"
#include "porytiles2/infra/services/ProjectCSourceFileAppender.hpp"
#include "porytiles2/infra/services/TextualCSourceGenerator.hpp"

using namespace porytiles2;

class ProjectCSourceFileAppenderTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a temporary directory for testing
    temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_test";
    std::filesystem::create_directories(temp_dir_);

    // Create project structure
    auto src_dir = temp_dir_ / "src" / "data" / "tilesets";
    std::filesystem::create_directories(src_dir);

    // Create test files
    CreateTestFile(src_dir / "graphics.h", "// Graphics header\n");
    CreateTestFile(src_dir / "headers.h", "// Headers header\n");
    CreateTestFile(src_dir / "metatiles.h", "// Metatiles header\n");

    // Create ProjectPaths and services
    project_paths_ = std::make_unique<ProjectPaths>(temp_dir_);
    auto generator = std::make_unique<TextualCSourceGenerator>();
    appender_ = std::make_unique<ProjectCSourceFileAppender>(
        gsl::not_null<ProjectPaths *>(project_paths_.get()), std::move(generator));
  }

  void TearDown() override {
    // Clean up temporary directory
    std::filesystem::remove_all(temp_dir_);
  }

  void CreateTestFile(const std::filesystem::path &file_path, const std::string &content) {
    std::ofstream file{file_path};
    file << content;
  }

  std::string ReadFile(const std::filesystem::path &file_path) {
    std::ifstream file{file_path};
    std::string content{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    return content;
  }

  std::filesystem::path temp_dir_;
  std::unique_ptr<ProjectPaths> project_paths_;
  std::unique_ptr<ProjectCSourceFileAppender> appender_;
};

TEST_F(ProjectCSourceFileAppenderTest, AppendToGraphicsHeaderShouldWork) {
  auto result = appender_->append_to_graphics_header("MyTileset");

  EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

  // Read the modified file
  const auto graphics_path = project_paths_->graphics_header();
  const auto content = ReadFile(graphics_path);

  // Should contain original content
  EXPECT_TRUE(content.find("// Graphics header") != std::string::npos);

  // Should contain palette declaration
  EXPECT_TRUE(content.find("gTilesetPalettes_MyTileset") != std::string::npos);

  // Should contain tile declaration
  EXPECT_TRUE(content.find("gTilesetTiles_MyTileset") != std::string::npos);

  // Should contain INCBIN macros
  EXPECT_TRUE(content.find("INCBIN_U16(") != std::string::npos);
  EXPECT_TRUE(content.find("INCBIN_U32(") != std::string::npos);
}

TEST_F(ProjectCSourceFileAppenderTest, AppendToHeadersHeaderShouldWork) {
  auto result = appender_->append_to_headers_header("MyTileset");

  EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

  // Read the modified file
  const auto headers_path = project_paths_->headers_header();
  const auto content = ReadFile(headers_path);

  // Should contain original content
  EXPECT_TRUE(content.find("// Headers header") != std::string::npos);

  // Should contain tileset struct
  EXPECT_TRUE(content.find("gTileset_MyTileset") != std::string::npos);
  EXPECT_TRUE(content.find("struct Tileset") != std::string::npos);

  // Should contain struct fields
  EXPECT_TRUE(content.find(".isCompressed = TRUE") != std::string::npos);
  EXPECT_TRUE(content.find(".isSecondary = FALSE") != std::string::npos);
}

TEST_F(ProjectCSourceFileAppenderTest, AppendToMetatilesHeaderShouldWork) {
  auto result = appender_->append_to_metatiles_header("MyTileset");

  EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

  // Read the modified file
  const auto metatiles_path = project_paths_->metatiles_header();
  const auto content = ReadFile(metatiles_path);

  // Should contain original content
  EXPECT_TRUE(content.find("// Metatiles header") != std::string::npos);

  // Should contain metatile declarations
  EXPECT_TRUE(content.find("gMetatiles_MyTileset") != std::string::npos);
  EXPECT_TRUE(content.find("gMetatileAttributes_MyTileset") != std::string::npos);

  // Should contain INCBIN macros
  EXPECT_TRUE(content.find("INCBIN_U16(") != std::string::npos);
}

TEST_F(ProjectCSourceFileAppenderTest, AppendTilesetDeclarationsShouldWork) {
  auto result = appender_->append_tileset_declarations("TestTileset");

  EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

  // Check all three files were modified
  const auto graphics_content = ReadFile(project_paths_->graphics_header());
  const auto headers_content = ReadFile(project_paths_->headers_header());
  const auto metatiles_content = ReadFile(project_paths_->metatiles_header());

  // Graphics file should contain palette and tile declarations
  EXPECT_TRUE(graphics_content.find("gTilesetPalettes_TestTileset") != std::string::npos);
  EXPECT_TRUE(graphics_content.find("gTilesetTiles_TestTileset") != std::string::npos);

  // Headers file should contain struct definition
  EXPECT_TRUE(headers_content.find("gTileset_TestTileset") != std::string::npos);

  // Metatiles file should contain metatile declarations
  EXPECT_TRUE(metatiles_content.find("gMetatiles_TestTileset") != std::string::npos);
  EXPECT_TRUE(metatiles_content.find("gMetatileAttributes_TestTileset") != std::string::npos);
}

TEST_F(ProjectCSourceFileAppenderTest, AppendToNonExistentFileShouldFail) {
  // Remove the graphics file
  std::filesystem::remove(project_paths_->graphics_header());

  auto result = appender_->append_to_graphics_header("MyTileset");

  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().find("Cannot open file for reading") != std::string::npos);
}

TEST_F(ProjectCSourceFileAppenderTest, AppendWithComplexTilesetNameShouldWork) {
  auto result = appender_->append_to_graphics_header("MyComplexTilesetName");

  EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

  const auto content = ReadFile(project_paths_->graphics_header());

  // Should preserve PascalCase in C variable names
  EXPECT_TRUE(content.find("gTilesetPalettes_MyComplexTilesetName") != std::string::npos);
  EXPECT_TRUE(content.find("gTilesetTiles_MyComplexTilesetName") != std::string::npos);

  // Should convert to lowercase with underscores in file paths
  EXPECT_TRUE(content.find("my_complex_tileset_name/palettes/") != std::string::npos);
  EXPECT_TRUE(content.find("my_complex_tileset_name/tiles.4bpp.lz") != std::string::npos);
}

TEST_F(ProjectCSourceFileAppenderTest, MultipleAppendsShouldAccumulate) {
  // Append first tileset
  auto result1 = appender_->append_to_graphics_header("TilesetOne");
  EXPECT_TRUE(result1.has_value());

  // Append second tileset
  auto result2 = appender_->append_to_graphics_header("TilesetTwo");
  EXPECT_TRUE(result2.has_value());

  const auto content = ReadFile(project_paths_->graphics_header());

  // Should contain both tilesets
  EXPECT_TRUE(content.find("gTilesetPalettes_TilesetOne") != std::string::npos);
  EXPECT_TRUE(content.find("gTilesetPalettes_TilesetTwo") != std::string::npos);
  EXPECT_TRUE(content.find("gTilesetTiles_TilesetOne") != std::string::npos);
  EXPECT_TRUE(content.find("gTilesetTiles_TilesetTwo") != std::string::npos);
}

TEST_F(ProjectCSourceFileAppenderTest, AppendTilesetDeclarationsFailureInMiddleShouldPropagate) {
  // Remove the headers file to cause a failure in the middle
  std::filesystem::remove(project_paths_->headers_header());

  auto result = appender_->append_tileset_declarations("TestTileset");

  EXPECT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().find("Failed to append to headers.h") != std::string::npos);

  // Graphics file should still be modified (operation is not atomic)
  const auto graphics_content = ReadFile(project_paths_->graphics_header());
  EXPECT_TRUE(graphics_content.find("gTilesetPalettes_TestTileset") != std::string::npos);
}
#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "fmt/format.h"

#include "porytiles2/infra/project/project_paths.hpp"
#include "porytiles2/infra/services/src_edit/project_c_source_file_appender.hpp"
#include "porytiles2/infra/services/src_edit/textual_c_source_generator.hpp"

using namespace porytiles2;

class ProjectCSourceFileAppenderIntegrationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Create a temporary directory for testing
        temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_appender_integration_test";
        std::filesystem::create_directories(temp_dir_);

        // Create realistic pokeemerald project structure
        create_project_structure();

        // Create ProjectPaths and services
        project_paths_ = std::make_unique<ProjectPaths>(temp_dir_);
        auto generator = std::make_unique<TextualCSourceGenerator>();
        appender_ =
            std::make_unique<ProjectCSourceFileAppender>(gsl::not_null(project_paths_.get()), std::move(generator));
    }

    void TearDown() override {
        // Clean up temporary directory
        std::filesystem::remove_all(temp_dir_);
    }

    void create_project_structure() {
        // Create directory structure
        auto src_dir = temp_dir_ / "src" / "data" / "tilesets";
        std::filesystem::create_directories(src_dir);

        // Create realistic graphics.h file
        create_graphics_header(src_dir / "graphics.h");

        // Create realistic headers.h file
        create_headers_header(src_dir / "headers.h");

        // Create realistic metatiles.h file
        create_metatiles_header(src_dir / "metatiles.h");
    }

    void create_graphics_header(const std::filesystem::path &file_path) {
        std::ofstream file{file_path};
        file << R"(#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "global.h"

// Existing tileset palettes
const u16 gTilesetPalettes_General[][16] = {
    INCBIN_U16("data/tilesets/primary/general/palettes/00.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/01.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/02.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/03.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/04.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/05.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/06.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/07.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/08.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/09.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/10.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/11.gbapal"),
    INCBIN_U16("data/tilesets/primary/general/palettes/12.gbapal"),
};

const u32 gTilesetTiles_General[] = INCBIN_U32("data/tilesets/primary/general/tiles.4bpp.lz");

#endif // GRAPHICS_H
)";
    }

    void create_headers_header(const std::filesystem::path &file_path) {
        std::ofstream file{file_path};
        file << R"(#ifndef HEADERS_H
#define HEADERS_H

#include "global.h"

// Existing tileset structs
const struct Tileset gTileset_General = {
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_General,
    .palettes = gTilesetPalettes_General,
    .metatiles = gMetatiles_General,
    .metatileAttributes = gMetatileAttributes_General,
    .callback = NULL,
};

#endif // HEADERS_H
)";
    }

    void create_metatiles_header(const std::filesystem::path &file_path) {
        std::ofstream file{file_path};
        file << R"(#ifndef METATILES_H
#define METATILES_H

#include "global.h"

// Existing metatile data
const u16 gMetatiles_General[] = INCBIN_U16("data/tilesets/primary/general/metatiles.bin");

const u16 gMetatileAttributes_General[] = INCBIN_U16("data/tilesets/primary/general/metatile_attributes.bin");

#endif // METATILES_H
)";
    }

    std::string read_file(const std::filesystem::path &file_path) {
        std::ifstream file{file_path};
        return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    void assert_file_contains(const std::filesystem::path &file_path, const std::string &content) {
        const auto file_content = read_file(file_path);
        EXPECT_TRUE(file_content.find(content) != std::string::npos)
            << "File " << file_path << " does not contain: " << content;
    }

    void assert_file_does_not_contain(const std::filesystem::path &file_path, const std::string &content) {
        const auto file_content = read_file(file_path);
        EXPECT_TRUE(file_content.find(content) == std::string::npos)
            << "File " << file_path << " unexpectedly contains: " << content;
    }

    std::filesystem::path temp_dir_;
    std::unique_ptr<ProjectPaths> project_paths_;
    std::unique_ptr<ProjectCSourceFileAppender> appender_;
};

TEST_F(ProjectCSourceFileAppenderIntegrationTest, AppendToGraphicsHeaderShouldIntegrateWithExistingContent) {
    // Verify existing content is present
    assert_file_contains(project_paths_->graphics_header(), "gTilesetPalettes_General");
    assert_file_contains(project_paths_->graphics_header(), "gTilesetTiles_General");

    // Append new tileset
    auto result = appender_->append_to_graphics_header("MyNewTileset");
    EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

    // Verify both old and new content exist
    assert_file_contains(project_paths_->graphics_header(), "gTilesetPalettes_General");
    assert_file_contains(project_paths_->graphics_header(), "gTilesetTiles_General");
    assert_file_contains(project_paths_->graphics_header(), "gTilesetPalettes_MyNewTileset");
    assert_file_contains(project_paths_->graphics_header(), "gTilesetTiles_MyNewTileset");

    // Verify proper file paths are generated
    assert_file_contains(project_paths_->graphics_header(), "my_new_tileset/palettes/00.gbapal");
    assert_file_contains(project_paths_->graphics_header(), "my_new_tileset/tiles.4bpp.lz");
}

TEST_F(ProjectCSourceFileAppenderIntegrationTest, AppendToHeadersHeaderShouldIntegrateWithExistingContent) {
    // Verify existing content is present
    assert_file_contains(project_paths_->headers_header(), "gTileset_General");

    // Append new tileset
    auto result = appender_->append_to_headers_header("MyNewTileset");
    EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

    // Verify both old and new content exist
    assert_file_contains(project_paths_->headers_header(), "gTileset_General");
    assert_file_contains(project_paths_->headers_header(), "gTileset_MyNewTileset");

    // Verify proper struct fields
    assert_file_contains(project_paths_->headers_header(), ".isCompressed = TRUE");
    assert_file_contains(project_paths_->headers_header(), ".isSecondary = FALSE");
    assert_file_contains(project_paths_->headers_header(), ".tiles = gTilesetTiles_MyNewTileset");
    assert_file_contains(project_paths_->headers_header(), ".palettes = gTilesetPalettes_MyNewTileset");
}

TEST_F(ProjectCSourceFileAppenderIntegrationTest, AppendToMetatilesHeaderShouldIntegrateWithExistingContent) {
    // Verify existing content is present
    assert_file_contains(project_paths_->metatiles_header(), "gMetatiles_General");
    assert_file_contains(project_paths_->metatiles_header(), "gMetatileAttributes_General");

    // Append new tileset
    auto result = appender_->append_to_metatiles_header("MyNewTileset");
    EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

    // Verify both old and new content exist
    assert_file_contains(project_paths_->metatiles_header(), "gMetatiles_General");
    assert_file_contains(project_paths_->metatiles_header(), "gMetatileAttributes_General");
    assert_file_contains(project_paths_->metatiles_header(), "gMetatiles_MyNewTileset");
    assert_file_contains(project_paths_->metatiles_header(), "gMetatileAttributes_MyNewTileset");

    // Verify proper file paths are generated
    assert_file_contains(project_paths_->metatiles_header(), "my_new_tileset/metatiles.bin");
    assert_file_contains(project_paths_->metatiles_header(), "my_new_tileset/metatile_attributes.bin");
}

TEST_F(ProjectCSourceFileAppenderIntegrationTest, AppendTilesetDeclarationsShouldModifyAllFiles) {
    // Verify initial state
    assert_file_does_not_contain(project_paths_->graphics_header(), "TestTileset");
    assert_file_does_not_contain(project_paths_->headers_header(), "TestTileset");
    assert_file_does_not_contain(project_paths_->metatiles_header(), "TestTileset");

    // Append complete tileset
    auto result = appender_->append_tileset_declarations("TestTileset");
    EXPECT_TRUE(result.has_value()) << "Expected success but got error: " << result.error();

    // Verify all files were modified
    assert_file_contains(project_paths_->graphics_header(), "gTilesetPalettes_TestTileset");
    assert_file_contains(project_paths_->graphics_header(), "gTilesetTiles_TestTileset");
    assert_file_contains(project_paths_->headers_header(), "gTileset_TestTileset");
    assert_file_contains(project_paths_->metatiles_header(), "gMetatiles_TestTileset");
    assert_file_contains(project_paths_->metatiles_header(), "gMetatileAttributes_TestTileset");

    // Verify existing content is preserved
    assert_file_contains(project_paths_->graphics_header(), "gTilesetPalettes_General");
    assert_file_contains(project_paths_->headers_header(), "gTileset_General");
    assert_file_contains(project_paths_->metatiles_header(), "gMetatiles_General");
}

TEST_F(ProjectCSourceFileAppenderIntegrationTest, MultipleAppendsShouldNotCorruptFiles) {
    // Append multiple tilesets
    auto result1 = appender_->append_tileset_declarations("TilesetOne");
    EXPECT_TRUE(result1.has_value());

    auto result2 = appender_->append_tileset_declarations("TilesetTwo");
    EXPECT_TRUE(result2.has_value());

    auto result3 = appender_->append_tileset_declarations("TilesetThree");
    EXPECT_TRUE(result3.has_value());

    // Verify all tilesets exist
    const std::vector<std::string> tilesets = {"General", "TilesetOne", "TilesetTwo", "TilesetThree"};

    for (const auto &tileset : tilesets) {
        assert_file_contains(project_paths_->graphics_header(), fmt::format("gTilesetPalettes_{}", tileset));
        assert_file_contains(project_paths_->graphics_header(), fmt::format("gTilesetTiles_{}", tileset));
        assert_file_contains(project_paths_->headers_header(), fmt::format("gTileset_{}", tileset));
        assert_file_contains(project_paths_->metatiles_header(), fmt::format("gMetatiles_{}", tileset));
        assert_file_contains(project_paths_->metatiles_header(), fmt::format("gMetatileAttributes_{}", tileset));
    }
}

TEST_F(ProjectCSourceFileAppenderIntegrationTest, AppendShouldPreserveFileStructure) {
    // Read original files
    const auto original_graphics = read_file(project_paths_->graphics_header());
    const auto original_headers = read_file(project_paths_->headers_header());
    const auto original_metatiles = read_file(project_paths_->metatiles_header());

    // Append new tileset
    auto result = appender_->append_tileset_declarations("NewTileset");
    EXPECT_TRUE(result.has_value());

    // Read modified files
    const auto modified_graphics = read_file(project_paths_->graphics_header());
    const auto modified_headers = read_file(project_paths_->headers_header());
    const auto modified_metatiles = read_file(project_paths_->metatiles_header());

    // Verify original content is preserved at the beginning
    EXPECT_TRUE(modified_graphics.starts_with(original_graphics));
    EXPECT_TRUE(modified_headers.starts_with(original_headers));
    EXPECT_TRUE(modified_metatiles.starts_with(original_metatiles));

    // Verify files are longer (new content was added)
    EXPECT_GT(modified_graphics.length(), original_graphics.length());
    EXPECT_GT(modified_headers.length(), original_headers.length());
    EXPECT_GT(modified_metatiles.length(), original_metatiles.length());
}

TEST_F(ProjectCSourceFileAppenderIntegrationTest, AppendShouldGenerateValidCCode) {
    auto result = appender_->append_tileset_declarations("ValidCodeTest");
    EXPECT_TRUE(result.has_value());

    // Check for valid C syntax patterns
    const auto graphics_content = read_file(project_paths_->graphics_header());
    const auto headers_content = read_file(project_paths_->headers_header());
    const auto metatiles_content = read_file(project_paths_->metatiles_header());

    // Verify array declarations are properly formatted
    EXPECT_TRUE(graphics_content.find("const u16 gTilesetPalettes_ValidCodeTest[][16] =") != std::string::npos);
    EXPECT_TRUE(graphics_content.find("const u32 gTilesetTiles_ValidCodeTest[] =") != std::string::npos);

    // Verify struct is properly formatted
    EXPECT_TRUE(headers_content.find("const struct Tileset gTileset_ValidCodeTest =") != std::string::npos);
    EXPECT_TRUE(headers_content.find("{\n") != std::string::npos);
    EXPECT_TRUE(headers_content.find("};") != std::string::npos);

    // Verify proper semicolons and braces
    EXPECT_TRUE(metatiles_content.find("gMetatiles_ValidCodeTest[] = INCBIN_U16(") != std::string::npos);
    EXPECT_TRUE(metatiles_content.find("gMetatileAttributes_ValidCodeTest[] = INCBIN_U16(") != std::string::npos);
}
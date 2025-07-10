#include "gtest/gtest.h"

#include "porytiles2/infra/services/textual_c_source_generator.hpp"

using namespace porytiles2;

class TextualCSourceGeneratorTest : public ::testing::Test {
protected:
  TextualCSourceGenerator generator;
};

TEST_F(TextualCSourceGeneratorTest, GeneratePaletteDeclarationShouldWork) {
  const std::string result = generator.generate_palette_declaration("MyTileset");

  // Should contain the proper array declaration
  EXPECT_TRUE(result.find("const u16 gTilesetPalettes_MyTileset[][16] =") != std::string::npos);

  // Should contain opening and closing braces
  EXPECT_TRUE(result.find("{\n") != std::string::npos);
  EXPECT_TRUE(result.find("\n};") != std::string::npos);

  // Should contain the lowercase path in includes
  EXPECT_TRUE(result.find("data/tilesets/primary/my_tileset/palettes/00.gbapal") !=
              std::string::npos);
  EXPECT_TRUE(result.find("data/tilesets/primary/my_tileset/palettes/12.gbapal") !=
              std::string::npos);

  // Should contain INCBIN_U16 macros
  EXPECT_TRUE(result.find("INCBIN_U16(") != std::string::npos);

  // Should have proper indentation for includes
  EXPECT_TRUE(result.find("    INCBIN_U16(") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, GeneratePaletteDeclarationWithComplexNameShouldWork) {
  const std::string result = generator.generate_palette_declaration("MyComplexTilesetName");

  // Should preserve PascalCase in C variable name
  EXPECT_TRUE(result.find("gTilesetPalettes_MyComplexTilesetName") != std::string::npos);

  // Should convert to lowercase with underscores in path
  EXPECT_TRUE(result.find("my_complex_tileset_name/palettes/") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, GenerateTileDeclarationShouldWork) {
  const std::string result = generator.generate_tile_declaration("MyTileset");

  // Should contain the proper array declaration with correct name
  EXPECT_TRUE(result.find("const u32 gTilesetTiles_MyTileset[] = ") != std::string::npos);

  // Should contain INCBIN_U32 macro
  EXPECT_TRUE(result.find("INCBIN_U32(") != std::string::npos);

  // Should contain correct path
  EXPECT_TRUE(result.find("data/tilesets/primary/my_tileset/tiles.4bpp.lz") != std::string::npos);

  // Should end with semicolon
  EXPECT_TRUE(result.find(");") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, GenerateTilesetStructDefinitionShouldWork) {
  const std::string result = generator.generate_tileset_struct_definition("MyTileset");

  // Should contain the struct declaration
  EXPECT_TRUE(result.find("const struct Tileset gTileset_MyTileset =") != std::string::npos);

  // Should contain opening and closing braces
  EXPECT_TRUE(result.find("{\n") != std::string::npos);
  EXPECT_TRUE(result.find("\n};") != std::string::npos);

  // Should contain all required fields
  EXPECT_TRUE(result.find(".isCompressed = TRUE") != std::string::npos);
  EXPECT_TRUE(result.find(".isSecondary = FALSE") != std::string::npos);
  EXPECT_TRUE(result.find(".tiles = gTilesetTiles_MyTileset") != std::string::npos);
  EXPECT_TRUE(result.find(".palettes = gTilesetPalettes_MyTileset") != std::string::npos);
  EXPECT_TRUE(result.find(".metatiles = gMetatiles_MyTileset") != std::string::npos);
  EXPECT_TRUE(result.find(".metatileAttributes = gMetatileAttributes_MyTileset") !=
              std::string::npos);
  EXPECT_TRUE(result.find(".callback = NULL") != std::string::npos);

  // Should have proper indentation
  EXPECT_TRUE(result.find("    .isCompressed = TRUE") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, GenerateMetatileDeclarationShouldWork) {
  const std::string result = generator.generate_metatile_declaration("MyTileset");

  // Should contain the proper array declaration
  EXPECT_TRUE(result.find("const u16 gMetatiles_MyTileset[] = ") != std::string::npos);

  // Should contain INCBIN_U16 macro
  EXPECT_TRUE(result.find("INCBIN_U16(") != std::string::npos);

  // Should contain correct path
  EXPECT_TRUE(result.find("data/tilesets/primary/my_tileset/metatiles.bin") != std::string::npos);

  // Should end with semicolon
  EXPECT_TRUE(result.find(");") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, GenerateMetatileAttributeDeclarationShouldWork) {
  const std::string result = generator.generate_metatile_attribute_declaration("MyTileset");

  // Should contain the proper array declaration
  EXPECT_TRUE(result.find("const u16 gMetatileAttributes_MyTileset[] = ") != std::string::npos);

  // Should contain INCBIN_U16 macro
  EXPECT_TRUE(result.find("INCBIN_U16(") != std::string::npos);

  // Should contain correct path
  EXPECT_TRUE(result.find("data/tilesets/primary/my_tileset/metatile_attributes.bin") !=
              std::string::npos);

  // Should end with semicolon
  EXPECT_TRUE(result.find(");") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, FormatWithIndentationShouldWork) {
  const std::string input = "line1\nline2\nline3";
  const std::string result = generator.format_with_indentation(input, 2);

  // Should have proper indentation (8 spaces = 2 levels * 4 spaces)
  EXPECT_TRUE(result.find("        line1") != std::string::npos);
  EXPECT_TRUE(result.find("        line2") != std::string::npos);
  EXPECT_TRUE(result.find("        line3") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, FormatWithIndentationZeroLevelShouldWork) {
  const std::string input = "line1\nline2";
  const std::string result = generator.format_with_indentation(input, 0);

  // Should return input unchanged
  EXPECT_EQ(result, input);
}

TEST_F(TextualCSourceGeneratorTest, FormatWithIndentationEmptyLinesShouldWork) {
  const std::string input = "line1\n\nline3";
  const std::string result = generator.format_with_indentation(input, 1);

  // Should indent non-empty lines but not empty lines
  EXPECT_TRUE(result.find("    line1") != std::string::npos);
  EXPECT_TRUE(result.find("    line3") != std::string::npos);

  // Should contain empty line without indentation
  EXPECT_TRUE(result.find("line1\n\n    line3") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, GenerateIncludeGuardsShouldWork) {
  const std::string result = generator.generate_include_guards("my_header");

  // Should contain proper include guards
  EXPECT_TRUE(result.find("#ifndef MY_HEADER_H") != std::string::npos);
  EXPECT_TRUE(result.find("#define MY_HEADER_H") != std::string::npos);
  EXPECT_TRUE(result.find("#endif // MY_HEADER_H") != std::string::npos);

  // Should have proper structure
  EXPECT_TRUE(result.find("#ifndef MY_HEADER_H\n#define MY_HEADER_H\n\n#endif // MY_HEADER_H") !=
              std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, GenerateIncludeGuardsWithUnderscoresShouldWork) {
  const std::string result = generator.generate_include_guards("my_complex_header");

  // Should convert to uppercase properly
  EXPECT_TRUE(result.find("MY_COMPLEX_HEADER_H") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, ToLowercaseFilePathConversionShouldWork) {
  // Test a simple case
  const std::string result1 = generator.generate_tile_declaration("Simple");
  EXPECT_TRUE(result1.find("simple/tiles.4bpp.lz") != std::string::npos);

  // Test PascalCase conversion
  const std::string result2 = generator.generate_tile_declaration("MyTileset");
  EXPECT_TRUE(result2.find("my_tileset/tiles.4bpp.lz") != std::string::npos);

  // Test complex PascalCase
  const std::string result3 = generator.generate_tile_declaration("VeryComplexTilesetName");
  EXPECT_TRUE(result3.find("very_complex_tileset_name/tiles.4bpp.lz") != std::string::npos);

  // Test a single character
  const std::string result4 = generator.generate_tile_declaration("A");
  EXPECT_TRUE(result4.find("a/tiles.4bpp.lz") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, PaletteIncludesShouldHaveCorrectCount) {
  const std::string result = generator.generate_palette_declaration("Test");

  // Should have exactly 13 palette entries (00-12)
  size_t count = 0;
  size_t pos = 0;
  while ((pos = result.find("INCBIN_U16(", pos)) != std::string::npos) {
    count++;
    pos += 11; // Length of "INCBIN_U16("
  }
  EXPECT_EQ(count, 13);

  // Should have specific palette files
  EXPECT_TRUE(result.find("palettes/00.gbapal") != std::string::npos);
  EXPECT_TRUE(result.find("palettes/05.gbapal") != std::string::npos);
  EXPECT_TRUE(result.find("palettes/12.gbapal") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, PaletteIncludesShouldHaveCorrectCommaPlacement) {
  const std::string result = generator.generate_palette_declaration("Test");

  // Should have commas after entries 0-11 but not after entry 12
  EXPECT_TRUE(result.find("palettes/00.gbapal\"),") != std::string::npos);
  EXPECT_TRUE(result.find("palettes/11.gbapal\"),") != std::string::npos);

  // Last entry should not have comma
  EXPECT_TRUE(result.find("palettes/12.gbapal\")") != std::string::npos);
  EXPECT_FALSE(result.find("palettes/12.gbapal\"),") != std::string::npos);
}

TEST_F(TextualCSourceGeneratorTest, AllMethodsShouldHandleEmptyInputGracefully) {
  // Test with empty string - should not crash
  EXPECT_NO_THROW(generator.generate_palette_declaration(""));
  EXPECT_NO_THROW(generator.generate_tile_declaration(""));
  EXPECT_NO_THROW(generator.generate_tileset_struct_definition(""));
  EXPECT_NO_THROW(generator.generate_metatile_declaration(""));
  EXPECT_NO_THROW(generator.generate_metatile_attribute_declaration(""));
  EXPECT_NO_THROW(generator.generate_include_guards(""));
  EXPECT_NO_THROW(generator.format_with_indentation("", 0));
  EXPECT_NO_THROW(generator.format_with_indentation("", 1));
}

TEST_F(TextualCSourceGeneratorTest, GeneratedCodeShouldBeWellFormed) {
  const std::string palette_decl = generator.generate_palette_declaration("TestTileset");
  const std::string tile_decl = generator.generate_tile_declaration("TestTileset");
  const std::string struct_def = generator.generate_tileset_struct_definition("TestTileset");
  const std::string metatile_decl = generator.generate_metatile_declaration("TestTileset");
  const std::string attr_decl = generator.generate_metatile_attribute_declaration("TestTileset");

  // All should have proper C syntax structure
  EXPECT_TRUE(palette_decl.find("const u16") != std::string::npos);
  EXPECT_TRUE(tile_decl.find("const u32") != std::string::npos);
  EXPECT_TRUE(struct_def.find("const struct") != std::string::npos);
  EXPECT_TRUE(metatile_decl.find("const u16") != std::string::npos);
  EXPECT_TRUE(attr_decl.find("const u16") != std::string::npos);

  // All should end with semicolon
  EXPECT_TRUE(palette_decl.back() == ';');
  EXPECT_TRUE(tile_decl.back() == ';');
  EXPECT_TRUE(struct_def.back() == ';');
  EXPECT_TRUE(metatile_decl.back() == ';');
  EXPECT_TRUE(attr_decl.back() == ';');
}

TEST_F(TextualCSourceGeneratorTest, ConstistentNamingConventionsShouldBeFollowed) {
  const std::string tileset_name = "MyTestTileset";
  const std::string palette_decl = generator.generate_palette_declaration(tileset_name);
  const std::string tile_decl = generator.generate_tile_declaration(tileset_name);
  const std::string struct_def = generator.generate_tileset_struct_definition(tileset_name);
  const std::string metatile_decl = generator.generate_metatile_declaration(tileset_name);
  const std::string attr_decl = generator.generate_metatile_attribute_declaration(tileset_name);

  // All should use a consistent naming pattern
  EXPECT_TRUE(palette_decl.find("gTilesetPalettes_MyTestTileset") != std::string::npos);
  EXPECT_TRUE(tile_decl.find("gTilesetTiles_MyTestTileset") != std::string::npos);
  EXPECT_TRUE(struct_def.find("gTileset_MyTestTileset") != std::string::npos);
  EXPECT_TRUE(metatile_decl.find("gMetatiles_MyTestTileset") != std::string::npos);
  EXPECT_TRUE(attr_decl.find("gMetatileAttributes_MyTestTileset") != std::string::npos);

  // File paths should use consistent lowercase with underscores
  EXPECT_TRUE(palette_decl.find("my_test_tileset/palettes/") != std::string::npos);
  EXPECT_TRUE(tile_decl.find("my_test_tileset/tiles.4bpp.lz") != std::string::npos);
  EXPECT_TRUE(metatile_decl.find("my_test_tileset/metatiles.bin") != std::string::npos);
  EXPECT_TRUE(attr_decl.find("my_test_tileset/metatile_attributes.bin") != std::string::npos);
}
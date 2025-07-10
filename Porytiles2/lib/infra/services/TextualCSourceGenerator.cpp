#include "porytiles2/infra/services/TextualCSourceGenerator.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <sstream>

#include "fmt/format.h"

namespace porytiles {

/// Number of spaces per indentation level
static constexpr int kSpacesPerIndent = 4;

/// Number of palette entries in a pokeemerald tileset
static constexpr int kNumPaletteEntries = 13;

std::string TextualCSourceGenerator::generate_palette_declaration(const std::string &tileset_name) {
  const auto tileset_path = ToLowercaseFilePath(tileset_name);
  const auto palette_includes = GenerateAllPaletteIncludes(tileset_path);

  return fmt::format("const u16 gTilesetPalettes_{}[][16] =\n"
                     "{{\n"
                     "{}\n"
                     "}};",
                     tileset_name, palette_includes);
}

std::string TextualCSourceGenerator::generate_tile_declaration(const std::string &tileset_name) {
  const auto tileset_path = ToLowercaseFilePath(tileset_name);

  return fmt::format(
      "const u32 gTilesetTiles_{}[] = INCBIN_U32(\"data/tilesets/primary/{}/tiles.4bpp.lz\");",
      tileset_name, tileset_path);
}

std::string
TextualCSourceGenerator::generate_tileset_struct_definition(const std::string &tileset_name) {
  return fmt::format("const struct Tileset gTileset_{} =\n"
                     "{{\n"
                     "    .isCompressed = TRUE,\n"
                     "    .isSecondary = FALSE,\n"
                     "    .tiles = gTilesetTiles_{},\n"
                     "    .palettes = gTilesetPalettes_{},\n"
                     "    .metatiles = gMetatiles_{},\n"
                     "    .metatileAttributes = gMetatileAttributes_{},\n"
                     "    .callback = NULL,\n"
                     "}};",
                     tileset_name, tileset_name, tileset_name, tileset_name, tileset_name);
}

std::string TextualCSourceGenerator::generate_metatile_declaration(const std::string &tileset_name) {
  const auto tileset_path = ToLowercaseFilePath(tileset_name);

  return fmt::format(
      "const u16 gMetatiles_{}[] = INCBIN_U16(\"data/tilesets/primary/{}/metatiles.bin\");",
      tileset_name, tileset_path);
}

std::string
TextualCSourceGenerator::generate_metatile_attribute_declaration(const std::string &tileset_name) {
  const auto tileset_path = ToLowercaseFilePath(tileset_name);

  return fmt::format("const u16 gMetatileAttributes_{}[] = "
                     "INCBIN_U16(\"data/tilesets/primary/{}/metatile_attributes.bin\");",
                     tileset_name, tileset_path);
}

std::string TextualCSourceGenerator::format_with_indentation(const std::string &code,
                                                           int indent_level) {
  return ApplyIndentation(code, indent_level);
}

std::string TextualCSourceGenerator::generate_include_guards(const std::string &header_name) {
  const auto uppercase_name = ToUppercase(header_name);

  return fmt::format("#ifndef {}_H\n"
                     "#define {}_H\n"
                     "\n"
                     "#endif // {}_H",
                     uppercase_name, uppercase_name, uppercase_name);
}

std::string TextualCSourceGenerator::ToLowercaseFilePath(const std::string &tileset_name) const {
  std::string result;
  result.reserve(tileset_name.length() + 10); // Reserve extra space for potential underscores

  for (size_t i = 0; i < tileset_name.length(); ++i) {
    const char c = tileset_name[i];

    // Add underscore before uppercase letters (except the first character)
    if (i > 0 && std::isupper(c)) {
      result += '_';
    }

    result += std::tolower(c);
  }

  return result;
}

std::string TextualCSourceGenerator::GeneratePaletteIncludeLine(const std::string &tileset_path,
                                                                int palette_index) const {
  return fmt::format("    INCBIN_U16(\"data/tilesets/primary/{}/palettes/{:02d}.gbapal\")",
                     tileset_path, palette_index);
}

std::string
TextualCSourceGenerator::GenerateAllPaletteIncludes(const std::string &tileset_path) const {
  std::ostringstream oss;

  for (int i = 0; i < kNumPaletteEntries; ++i) {
    oss << GeneratePaletteIncludeLine(tileset_path, i);

    // Add comma for all entries except the last one
    if (i < kNumPaletteEntries - 1) {
      oss << ",";
    }

    // Add a newline for all entries except the last one
    if (i < kNumPaletteEntries - 1) {
      oss << "\n";
    }
  }

  return oss.str();
}

std::string TextualCSourceGenerator::ApplyIndentation(const std::string &text,
                                                      int indent_level) const {
  if (indent_level <= 0) {
    return text;
  }

  const std::string indent(indent_level * kSpacesPerIndent, ' ');
  std::ostringstream oss;
  std::istringstream iss{text};
  std::string line;
  bool first_line = true;

  while (std::getline(iss, line)) {
    if (!first_line) {
      oss << "\n";
    }

    // Don't indent empty lines
    if (!line.empty()) {
      oss << indent << line;
    }

    first_line = false;
  }

  return oss.str();
}

std::string TextualCSourceGenerator::ToUppercase(const std::string &input) const {
  std::string result = input;
  std::ranges::transform(result, result.begin(), [](unsigned char c) { return std::toupper(c); });
  return result;
}

} // namespace porytiles
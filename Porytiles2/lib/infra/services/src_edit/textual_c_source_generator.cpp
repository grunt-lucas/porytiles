#include "porytiles2/infra/services/src_edit/textual_c_source_generator.hpp"

#include <algorithm>
#include <cctype>
#include <format>
#include <sstream>

#include "fmt/format.h"

namespace {
/// Number of spaces per indentation level
constexpr int kSpacesPerIndent = 4;

/// Number of palette entries in a pokeemerald tileset
constexpr int kNumPaletteEntries = 13;

std::string to_lowercase_file_path(const std::string &tileset_name) {
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

std::string generate_palette_include_line(const std::string &tileset_path, int palette_index) {
    return fmt::format("    INCBIN_U16(\"data/tilesets/primary/{}/palettes/{:02d}.gbapal\")", tileset_path,
                       palette_index);
}

std::string generate_all_palette_includes(const std::string &tileset_path) {
    std::ostringstream oss;

    for (int i = 0; i < kNumPaletteEntries; ++i) {
        oss << generate_palette_include_line(tileset_path, i);

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

std::string apply_indentation(const std::string &text, int indent_level) {
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

std::string to_uppercase(const std::string &input) {
    std::string result = input;
    std::ranges::transform(result, result.begin(), [](unsigned char c) { return std::toupper(c); });
    return result;
}

} // namespace

namespace porytiles2 {

std::string TextualCSourceGenerator::generate_palette_declaration(const std::string &tileset_name) {
    const auto tileset_path = to_lowercase_file_path(tileset_name);
    const auto palette_includes = generate_all_palette_includes(tileset_path);

    return fmt::format("const u16 gTilesetPalettes_{}[][16] =\n"
                       "{{\n"
                       "{}\n"
                       "}};",
                       tileset_name, palette_includes);
}

std::string TextualCSourceGenerator::generate_tile_declaration(const std::string &tileset_name) {
    const auto tileset_path = to_lowercase_file_path(tileset_name);

    return fmt::format("const u32 gTilesetTiles_{}[] = INCBIN_U32(\"data/tilesets/primary/{}/tiles.4bpp.lz\");",
                       tileset_name, tileset_path);
}

std::string TextualCSourceGenerator::generate_tileset_struct_definition(const std::string &tileset_name) {
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
    const auto tileset_path = to_lowercase_file_path(tileset_name);

    return fmt::format("const u16 gMetatiles_{}[] = INCBIN_U16(\"data/tilesets/primary/{}/metatiles.bin\");",
                       tileset_name, tileset_path);
}

std::string TextualCSourceGenerator::generate_metatile_attribute_declaration(const std::string &tileset_name) {
    const auto tileset_path = to_lowercase_file_path(tileset_name);

    return fmt::format("const u16 gMetatileAttributes_{}[] = "
                       "INCBIN_U16(\"data/tilesets/primary/{}/metatile_attributes.bin\");",
                       tileset_name, tileset_path);
}

std::string TextualCSourceGenerator::format_with_indentation(const std::string &code, int indent_level) {
    return apply_indentation(code, indent_level);
}

std::string TextualCSourceGenerator::generate_include_guards(const std::string &header_name) {
    const auto uppercase_name = to_uppercase(header_name);

    return fmt::format("#ifndef {}_H\n"
                       "#define {}_H\n"
                       "\n"
                       "#endif // {}_H",
                       uppercase_name, uppercase_name, uppercase_name);
}

} // namespace porytiles2

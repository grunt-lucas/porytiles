#pragma once

#include <string>

namespace porytiles2 {

/**
 * @brief Interface for generating C source code constructs related to tileset integration.
 *
 * @details
 * This interface provides a clean abstraction for generating C source code without
 * coupling to specific file operations or project structure. It focuses purely on
 * code generation logic, allowing different implementation strategies (text-based,
 * AST-based, etc.) to be used interchangeably.
 *
 * The generated code follows pokeemerald project conventions and integrates with
 * the existing C source files that define tileset structures and data.
 */
class CSourceGenerator {
public:
  virtual ~CSourceGenerator() = default;

  /**
   * @brief Generate palette data declaration for a tileset.
   *
   * Creates a C declaration for tileset palette data, including all 13 palette
   * entries (00-12) with proper INCBIN_U16 macros pointing to .gbapal files.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C declaration string for palette data
   */
  virtual std::string generate_palette_declaration(const std::string &tileset_name) = 0;

  /**
   * @brief Generate tile data declaration for a tileset.
   *
   * @details
   * Creates a C declaration for tileset tile data using INCBIN_U32 macro
   * pointing to the compressed tile data file.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C declaration string for tile data
   */
  virtual std::string generate_tile_declaration(const std::string &tileset_name) = 0;

  /**
   * @brief Generate tileset struct definition.
   *
   * @details
   * Creates a complete C struct definition for a primary tileset, including
   * all required fields (isCompressed, isSecondary, tiles, palettes, metatiles,
   * metatileAttributes, callback) with proper references to the data arrays.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C struct definition string
   */
  virtual std::string generate_tileset_struct_definition(const std::string &tileset_name) = 0;

  /**
   * @brief Generate metatile data declaration.
   *
   * @details
   * Creates a C declaration for metatile data using INCBIN_U16 macro
   * pointing to the metatiles.bin file.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C declaration string for metatile data
   */
  virtual std::string generate_metatile_declaration(const std::string &tileset_name) = 0;

  /**
   * @brief Generate metatile attribute data declaration.
   *
   * @details
   * Creates a C declaration for metatile attribute data using INCBIN_U16 macro
   * pointing to the metatile_attributes.bin file.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C declaration string for metatile attribute data
   */
  virtual std::string generate_metatile_attribute_declaration(const std::string &tileset_name) = 0;

  /**
   * @brief Format code with proper indentation.
   *
   * @details
   * Applies consistent indentation to C code using spaces (4 spaces per level).
   * This ensures generated code follows project formatting conventions.
   *
   * @param code The C code to format
   * @param indent_level The indentation level (0 = no indentation)
   * @return Formatted code string with proper indentation
   */
  virtual std::string format_with_indentation(const std::string &code, int indent_level) = 0;

  /**
   * @brief Generate include guards for header files.
   *
   * @details
   * Creates standard C include guards using the conventional pattern.
   * This is useful when generating new header files or header sections.
   *
   * @param header_name The name of the header file (without extension)
   * @return Include guard string with proper formatting
   */
  virtual std::string generate_include_guards(const std::string &header_name) = 0;
};

} // namespace porytiles2
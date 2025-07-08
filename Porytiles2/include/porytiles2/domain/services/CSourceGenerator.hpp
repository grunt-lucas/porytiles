#pragma once

#include <string>

namespace porytiles {

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
   *
   * @example
   * ```cpp
   * auto code = generator->GeneratePaletteDeclaration("MyTileset");
   * // Returns:
   * // const u16 gTilesetPalettes_MyTileset[][16] =
   * // {
   * //     INCBIN_U16("data/tilesets/primary/my_tileset/palettes/00.gbapal"),
   * //     INCBIN_U16("data/tilesets/primary/my_tileset/palettes/01.gbapal"),
   * //     ...
   * // };
   * ```
   */
  virtual std::string GeneratePaletteDeclaration(const std::string &tileset_name) = 0;

  /**
   * @brief Generate tile data declaration for a tileset.
   *
   * Creates a C declaration for tileset tile data using INCBIN_U32 macro
   * pointing to the compressed tile data file.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C declaration string for tile data
   *
   * @example
   * ```cpp
   * auto code = generator->GenerateTileDeclaration("MyTileset");
   * // Returns:
   * // const u32 gTilesetTiles_MyTileset[] =
   * INCBIN_U32("data/tilesets/primary/my_tileset/tiles.4bpp.lz");
   * ```
   */
  virtual std::string GenerateTileDeclaration(const std::string &tileset_name) = 0;

  /**
   * @brief Generate tileset struct definition.
   *
   * Creates a complete C struct definition for a primary tileset, including
   * all required fields (isCompressed, isSecondary, tiles, palettes, metatiles,
   * metatileAttributes, callback) with proper references to the data arrays.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C struct definition string
   *
   * @example
   * ```cpp
   * auto code = generator->GenerateTilesetStructDefinition("MyTileset");
   * // Returns:
   * // const struct Tileset gTileset_MyTileset =
   * // {
   * //     .isCompressed = TRUE,
   * //     .isSecondary = FALSE,
   * //     .tiles = gTilesetTiles_MyTileset,
   * //     .palettes = gTilesetPalettes_MyTileset,
   * //     .metatiles = gMetatiles_MyTileset,
   * //     .metatileAttributes = gMetatileAttributes_MyTileset,
   * //     .callback = NULL,
   * // };
   * ```
   */
  virtual std::string GenerateTilesetStructDefinition(const std::string &tileset_name) = 0;

  /**
   * @brief Generate metatile data declaration.
   *
   * Creates a C declaration for metatile data using INCBIN_U16 macro
   * pointing to the metatiles.bin file.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C declaration string for metatile data
   *
   * @example
   * ```cpp
   * auto code = generator->GenerateMetatileDeclaration("MyTileset");
   * // Returns:
   * // const u16 gMetatiles_MyTileset[] =
   * INCBIN_U16("data/tilesets/primary/my_tileset/metatiles.bin");
   * ```
   */
  virtual std::string GenerateMetatileDeclaration(const std::string &tileset_name) = 0;

  /**
   * @brief Generate metatile attribute data declaration.
   *
   * Creates a C declaration for metatile attribute data using INCBIN_U16 macro
   * pointing to the metatile_attributes.bin file.
   *
   * @param tileset_name The name of the tileset (e.g., "MyTileset")
   * @return Complete C declaration string for metatile attribute data
   *
   * @example
   * ```cpp
   * auto code = generator->GenerateMetatileAttributeDeclaration("MyTileset");
   * // Returns:
   * // const u16 gMetatileAttributes_MyTileset[] =
   * INCBIN_U16("data/tilesets/primary/my_tileset/metatile_attributes.bin");
   * ```
   */
  virtual std::string GenerateMetatileAttributeDeclaration(const std::string &tileset_name) = 0;

  /**
   * @brief Format code with proper indentation.
   *
   * Applies consistent indentation to C code using spaces (4 spaces per level).
   * This ensures generated code follows project formatting conventions.
   *
   * @param code The C code to format
   * @param indent_level The indentation level (0 = no indentation)
   * @return Formatted code string with proper indentation
   *
   * @example
   * ```cpp
   * auto formatted = generator->FormatWithIndentation("int x = 5;", 1);
   * // Returns: "    int x = 5;"
   * ```
   */
  virtual std::string FormatWithIndentation(const std::string &code, int indent_level) = 0;

  /**
   * @brief Generate include guards for header files.
   *
   * Creates standard C include guards using the conventional pattern.
   * This is useful when generating new header files or header sections.
   *
   * @param header_name The name of the header file (without extension)
   * @return Include guard string with proper formatting
   *
   * @example
   * ```cpp
   * auto guard = generator->GenerateIncludeGuards("my_header");
   * // Returns:
   * // #ifndef MY_HEADER_H
   * // #define MY_HEADER_H
   * //
   * // #endif // MY_HEADER_H
   * ```
   */
  virtual std::string GenerateIncludeGuards(const std::string &header_name) = 0;
};

} // namespace porytiles
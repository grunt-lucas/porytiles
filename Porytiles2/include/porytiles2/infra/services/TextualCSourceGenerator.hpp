#pragma once

#include <string>

#include "porytiles2/domain/services/CSourceGenerator.hpp"

namespace porytiles {

/**
 * @brief Text-based implementation of CSourceGenerator for pokeemerald projects.
 *
 * @details
 * This implementation uses string formatting and templates to generate C source
 * code constructs. It follows pokeemerald project conventions for tileset
 * integration and produces code that matches the existing patterns in the
 * codebase.
 *
 * The generator uses fmt::format for string templating and produces syntactically
 * correct C code that integrates seamlessly with pokeemerald's build system.
 *
 * @note This implementation assumes primary tilesets. Secondary tileset support may require
 * additional methods or parameters.
 */
class TextualCSourceGenerator final : public CSourceGenerator {
public:
  /**
   * @brief Default constructor.
   *
   * Creates a new TextualCSourceGenerator with default formatting settings.
   */
  TextualCSourceGenerator() = default;

  /**
   * @brief Virtual destructor.
   */
  ~TextualCSourceGenerator() override = default;

  // CSourceGenerator interface implementation
  std::string generate_palette_declaration(const std::string &tileset_name) override;
  std::string generate_tile_declaration(const std::string &tileset_name) override;
  std::string generate_tileset_struct_definition(const std::string &tileset_name) override;
  std::string generate_metatile_declaration(const std::string &tileset_name) override;
  std::string generate_metatile_attribute_declaration(const std::string &tileset_name) override;
  std::string format_with_indentation(const std::string &code, int indent_level) override;
  std::string generate_include_guards(const std::string &header_name) override;

private:
  /**
   * @brief Convert tileset name to lowercase for file paths.
   *
   * @details
   * Converts PascalCase tileset names to lowercase for use in file paths.
   * This follows pokeemerald's naming conventions where tileset names in
   * C code are PascalCase but file paths are lowercase.
   *
   * @param tileset_name The tileset name in PascalCase
   * @return Lowercase version for file paths
   */
  std::string ToLowercaseFilePath(const std::string &tileset_name) const;

  /**
   * @brief Generate individual palette include line.
   *
   * @details
   * Creates a single INCBIN_U16 line for a palette file with proper
   * indentation and formatting.
   *
   * @param tileset_path The lowercase tileset path
   * @param palette_index The palette index (0-12)
   * @return Formatted include line
   */
  std::string GeneratePaletteIncludeLine(const std::string &tileset_path, int palette_index) const;

  /**
   * @brief Generate all palette include lines.
   *
   * @details
   * Creates all 13 palette include lines (00-12) with proper formatting
   * and comma placement.
   *
   * @param tileset_path The lowercase tileset path
   * @return All palette include lines as a single string
   */
  std::string GenerateAllPaletteIncludes(const std::string &tileset_path) const;

  /**
   * @brief Apply indentation to a string.
   *
   * @details
   * Adds the specified number of indentation levels to each line of the
   * input string. Uses 4 spaces per indentation level.
   *
   * @param text The text to indent
   * @param indent_level The number of indentation levels
   * @return Indented text
   */
  std::string ApplyIndentation(const std::string &text, int indent_level) const;

  /**
   * @brief Convert string to uppercase.
   *
   * @details
   * Utility function to convert a string to uppercase for use in
   * include guards and other C preprocessor constructs.
   *
   * @param input The input string
   * @return Uppercase version of the input
   */
  std::string ToUppercase(const std::string &input) const;
};

} // namespace porytiles
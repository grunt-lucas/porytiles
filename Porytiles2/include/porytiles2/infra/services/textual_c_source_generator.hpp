#pragma once

#include <string>

#include "porytiles2/domain/services/c_source_generator.hpp"

namespace porytiles2 {

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
   * @brief Virtual destructor.
   */
  ~TextualCSourceGenerator() override = default;

  /**
   * @brief Default constructor.
   *
   * Creates a new TextualCSourceGenerator with default formatting settings.
   */
  TextualCSourceGenerator() = default;

  // CSourceGenerator interface implementation
  std::string generate_palette_declaration(const std::string &tileset_name) override;
  std::string generate_tile_declaration(const std::string &tileset_name) override;
  std::string generate_tileset_struct_definition(const std::string &tileset_name) override;
  std::string generate_metatile_declaration(const std::string &tileset_name) override;
  std::string generate_metatile_attribute_declaration(const std::string &tileset_name) override;
  std::string format_with_indentation(const std::string &code, int indent_level) override;
  std::string generate_include_guards(const std::string &header_name) override;
};

} // namespace porytiles2
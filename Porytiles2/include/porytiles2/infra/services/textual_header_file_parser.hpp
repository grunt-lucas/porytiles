#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "porytiles2/domain/services/header_file_parser.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Text-based implementation of HeaderFileParser for C header files.
 *
 * @details
 * This class provides a concrete implementation of the HeaderFileParser interface
 * using simple text-based parsing operations. It reads C header files line by line
 * and uses string pattern matching to understand file structure and find
 * appropriate insertion points.
 *
 * The implementation focuses on the specific needs of tileset integration:
 * - Finding safe locations to append new declarations
 * - Detecting existing declarations to prevent duplicates
 * - Basic validation of file structure
 * - Simple parsing without full AST analysis
 *
 * This approach is sufficient for the pokeemerald project structure and provides
 * a good balance between simplicity and functionality.
 */
class TextualHeaderFileParser final : public HeaderFileParser {
public:
  TextualHeaderFileParser() = default;

  /**
   * @brief Parse a C header file and extract its structure.
   *
   * @details
   * Reads the header file line by line and returns the contents as a vector
   * of strings. Each line is preserved exactly as it appears in the file,
   * including whitespace and comments.
   *
   * @param file_path The path to the C header file to parse
   * @return Result<std::vector<std::string>> containing the file lines or error details
   */
  [[nodiscard]] Result<std::vector<std::string>>
  parse_header_file(const std::filesystem::path &file_path) override;

  /**
   * @brief Check if a header file contains a specific declaration.
   *
   * @details
   * Searches through the header file content using simple string matching
   * to determine if a specific declaration pattern exists. This is used
   * to prevent duplicate declarations when modifying files.
   *
   * @param file_path The path to the C header file to check
   * @param declaration_pattern The pattern to search for (e.g., "gTileset_MyTileset")
   * @return Result<bool> indicating if the declaration exists or error details
   */
  [[nodiscard]] Result<bool> contains_declaration(const std::filesystem::path &file_path,
                                                  const std::string &declaration_pattern) override;

  /**
   * @brief Find the best insertion point for new declarations.
   *
   * @details
   * Analyzes the header file to find the end of the file content, typically
   * just before any closing include guards. For simple appending operations,
   * this returns the end of the file as the insertion point.
   *
   * @param file_path The path to the C header file to analyze
   * @return Result<size_t> containing the line number for insertion or error details
   */
  [[nodiscard]] Result<size_t>
  find_insertion_point(const std::filesystem::path &file_path) override;

  /**
   * @brief Validate that a header file has proper C syntax structure.
   *
   * @details
   * Performs basic validation including:
   * - File exists and is readable
   * - Contains valid C-style content
   * - Has reasonable structure for appending
   *
   * This is a simplified validation suitable for the appending use case.
   *
   * @param file_path The path to the C header file to validate
   * @return Result<bool> indicating if the file is valid or error details
   */
  [[nodiscard]] Result<bool>
  validate_header_structure(const std::filesystem::path &file_path) override;
};

} // namespace porytiles2
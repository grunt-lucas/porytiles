#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Service for parsing C header files to support modification operations.
 *
 * @details
 * This service provides functionality for parsing existing C header files
 * to understand their structure and find appropriate insertion points for
 * new declarations. It supports basic parsing operations needed for tileset
 * integration without necessarily requiring full AST parsing.
 *
 * The parser handles common C header file patterns including:
 * - Include guards
 * - Forward declarations
 * - Function declarations
 * - Variable declarations
 * - Struct definitions
 * - Comments and whitespace
 *
 * This service is designed to work with the existing pokeemerald project
 * structure and can identify suitable locations for appending new tileset
 * declarations.
 */
class HeaderFileParser {
  public:
    virtual ~HeaderFileParser() = default;

    /**
     * @brief Parse a C header file and extract its structure.
     *
     * @details
     * Reads and parses the specified C header file, extracting information
     * about its structure including existing declarations, include guards,
     * and potential insertion points for new content.
     *
     * @param file_path The path to the C header file to parse
     * @return Result<std::vector<std::string>> containing the file lines or error details
     */
    [[nodiscard]] virtual Result<std::vector<std::string>>
    parse_header_file(const std::filesystem::path &file_path) = 0;

    /**
     * @brief Check if a header file contains a specific declaration.
     *
     * @details
     * Searches through the header file content to determine if a specific
     * declaration (such as a tileset struct or data array) already exists.
     * This helps prevent duplicate declarations when modifying files.
     *
     * @param file_path The path to the C header file to check
     * @param declaration_pattern The pattern to search for (e.g., "gTileset_MyTileset")
     * @return Result<bool> indicating if the declaration exists or error details
     */
    [[nodiscard]] virtual Result<bool> contains_declaration(const std::filesystem::path &file_path,
                                                            const std::string &declaration_pattern) = 0;

    /**
     * @brief Find the best insertion point for new declarations.
     *
     * @details
     * Analyzes the header file structure to find the most appropriate location
     * for inserting new declarations. This typically involves finding the end
     * of existing declarations but before any closing include guards.
     *
     * @param file_path The path to the C header file to analyze
     * @return Result<size_t> containing the line number for insertion or error details
     */
    [[nodiscard]] virtual Result<size_t> find_insertion_point(const std::filesystem::path &file_path) = 0;

    /**
     * @brief Validate that a header file has proper C syntax structure.
     *
     * @details
     * Performs basic validation of the header file to ensure it has proper
     * C syntax structure including matching braces, proper include guards,
     * and valid declaration syntax. This helps ensure that modifications
     * won't corrupt the file.
     *
     * @param file_path The path to the C header file to validate
     * @return Result<bool> indicating if the file is valid or error details
     */
    [[nodiscard]] virtual Result<bool> validate_header_structure(const std::filesystem::path &file_path) = 0;
};

} // namespace porytiles2
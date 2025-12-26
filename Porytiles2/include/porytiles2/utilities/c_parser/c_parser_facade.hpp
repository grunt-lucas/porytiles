#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/utilities/c_parser/array_declaration.hpp"
#include "porytiles2/utilities/c_parser/c_parser_context.hpp"
#include "porytiles2/utilities/c_parser/define_statement.hpp"
#include "porytiles2/utilities/c_parser/enum_declaration.hpp"
#include "porytiles2/utilities/c_parser/function_definition.hpp"
#include "porytiles2/utilities/c_parser/struct_variable_declaration.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief High-level facade for parsing C/C++ source files.
 *
 * @details
 * CParserFacade orchestrates the complete parsing pipeline: file loading, lexing, and parsing. It owns the file
 * content and provides rich error formatting through FileHighlightPrinter integration.
 *
 * The facade provides a simple interface for extracting specific constructs from C/C++ source files:
 * - parse_defines() extracts all #define preprocessor directives
 * - parse_enums() extracts all enum declarations
 *
 * Error handling uses ChainableResult with FormattableError, providing multi-line error messages with source
 * code context highlighting showing exactly where errors occurred.
 *
 * Example usage:
 * @code
 * PlainTextFormatter formatter;
 * CParserFacade driver{"include/constants.h", &formatter};
 *
 * auto defines_result = driver.parse_defines();
 * if (!defines_result.has_value()) {
 *     // Error chain contains rich formatted output with source highlighting
 *     for (const auto& err : defines_result.chain()) {
 *         for (const auto& line : err->details(formatter)) {
 *             std::cerr << line << '\n';
 *         }
 *     }
 * }
 * @endcode
 */
class CParserFacade {
  public:
    /**
     * @brief Constructs a facade for parsing the specified file.
     *
     * @details
     * The file is not loaded until a parse method is called. This allows for efficient construction when the facade
     * may not be used, or when multiple facades are created but only some are actually needed.
     *
     * @param file_path Path to the C/C++ source file to parse
     * @param format Formatter for error message styling (non-owning, must outlive facade)
     */
    CParserFacade(std::filesystem::path file_path, gsl::not_null<const TextFormatter *> format);

    /**
     * @brief Parses all #define statements from the file.
     *
     * @details
     * Loads the file (if not already loaded), tokenizes it, and extracts all #define preprocessor directives. Returns
     * a vector of DefineStatement objects containing the macro names and evaluated values.
     *
     * On error (file not found, lexer error, parser error), returns a ChainableResult containing a FormattableError
     * with multi-line source context highlighting.
     *
     * @return A vector of DefineStatement on success, or an error chain on failure
     */
    [[nodiscard]] ChainableResult<std::vector<DefineStatement>> parse_defines();

    /**
     * @brief Parses all enum declarations from the file.
     *
     * @details
     * Loads the file (if not already loaded), tokenizes it, and extracts all enum declarations. Returns a vector of
     * EnumDeclaration objects containing the enum names and members with their values.
     *
     * On error (file not found, lexer error, parser error), returns a ChainableResult containing a FormattableError
     * with multi-line source context highlighting.
     *
     * @return A vector of EnumDeclaration on success, or an error chain on failure
     */
    [[nodiscard]] ChainableResult<std::vector<EnumDeclaration>> parse_enums();

    /**
     * @brief Parses all pointer array declarations from the file.
     *
     * @details
     * Loads the file (if not already loaded), tokenizes it, and extracts all pointer array declarations. Returns a
     * vector of ArrayDeclaration objects containing the array names and their initializer elements.
     *
     * This is used by AnimCodeParser to extract animation frame arrays like:
     * @code
     * const u16 *const gTilesetAnims_General_Flower[] = {
     *     gTilesetAnims_General_Flower_Frame0,
     *     gTilesetAnims_General_Flower_Frame1
     * };
     * @endcode
     *
     * On error (file not found, lexer error, parser error), returns a ChainableResult containing a FormattableError
     * with multi-line source context highlighting.
     *
     * @param name_prefix Optional prefix to filter array names. If provided, only arrays whose names start with this
     *        prefix are returned. For example, "gTilesetAnims_General_" would match only General tileset arrays.
     * @return A vector of ArrayDeclaration on success, or an error chain on failure
     */
    [[nodiscard]] ChainableResult<std::vector<ArrayDeclaration>>
    parse_pointer_arrays(const std::optional<std::string> &name_prefix = std::nullopt);

    /**
     * @brief Parses function definitions from the file.
     *
     * @details
     * Loads the file (if not already loaded), tokenizes it, and extracts function definitions. Returns a vector of
     * FunctionDefinition objects containing the function names and body tokens.
     *
     * This is used by AnimCodeParser to extract queue and driver functions like:
     * @code
     * static void QueueAnimTiles_General_Flower(u16 timer) {
     *     AppendTilesetAnimToBuffer(..., TILE_OFFSET_4BPP(12), 4 * TILE_SIZE_4BPP);
     * }
     * @endcode
     *
     * On error (file not found, lexer error, parser error), returns a ChainableResult containing a FormattableError
     * with multi-line source context highlighting.
     *
     * @param name_prefix Optional prefix to filter function names. If provided, only functions whose names start with
     *        this prefix are returned. For example, "QueueAnimTiles_" would match only queue functions.
     * @return A vector of FunctionDefinition on success, or an error chain on failure
     */
    [[nodiscard]] ChainableResult<std::vector<FunctionDefinition>>
    parse_functions(const std::optional<std::string> &name_prefix = std::nullopt);

    /**
     * @brief Parses struct variable declarations from the file.
     *
     * @details
     * Loads the file (if not already loaded), tokenizes it, and extracts struct variable declarations. Returns a vector
     * of StructVariableDeclaration objects containing the struct type and variable names.
     *
     * This is used by ProjectTilesetNameProvider to extract tileset names from headers.h files like:
     * @code
     * const struct Tileset gTileset_General = {
     *     .isCompressed = TRUE,
     *     .isSecondary = FALSE,
     *     ...
     * };
     * @endcode
     *
     * On error (file not found, lexer error, parser error), returns a ChainableResult containing a FormattableError
     * with multi-line source context highlighting.
     *
     * @param name_prefix Optional prefix to filter variable names. If provided, only struct variables whose names start
     *        with this prefix are returned. For example, "gTileset_" would match only tileset declarations.
     * @return A vector of StructVariableDeclaration on success, or an error chain on failure
     */
    [[nodiscard]] ChainableResult<std::vector<StructVariableDeclaration>>
    parse_struct_variables(const std::optional<std::string> &name_prefix = std::nullopt);

    /**
     * @brief Finds a specific #define statement by name.
     *
     * @details
     * Searches for a #define directive with the specified name. On first call, parses all defines and caches them
     * for efficient subsequent lookups. Returns std::nullopt if the define is not found (which is not an error).
     *
     * @param define_name The name of the #define macro to find
     * @return The DefineStatement if found, std::nullopt if not found, or an error on parse failure
     */
    [[nodiscard]] ChainableResult<std::optional<DefineStatement>> find_define(const std::string &define_name);

    /**
     * @brief Returns the cached file lines.
     *
     * @details
     * Returns a const reference to the file lines loaded during parsing. If the file has not been loaded yet (no parse
     * method called), returns an empty vector.
     *
     * @return Const reference to the file lines vector
     */
    [[nodiscard]] const std::vector<std::string> &file_lines() const;

  private:
    [[nodiscard]] ChainableResult<void> ensure_loaded();

    std::filesystem::path file_path_;
    const TextFormatter *format_;
    std::vector<std::string> file_lines_;
    std::string content_;
    std::unique_ptr<CParserContext> context_;
    bool loaded_{false};
    bool load_failed_{false};
    FormattableError load_error_;
    std::optional<std::vector<DefineStatement>> cached_defines_;
};

} // namespace porytiles2

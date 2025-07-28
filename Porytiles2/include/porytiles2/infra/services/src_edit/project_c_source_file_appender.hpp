#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/services/c_source_file_modifier.hpp"
#include "porytiles2/domain/services/c_source_generator.hpp"
#include "porytiles2/infra/project/project_paths.hpp"
#include "porytiles2/templates/result.hpp"

#include <gsl/gsl>

namespace porytiles2 {

/**
 * @brief String-based implementation of CSourceFileModifier for pokeemerald projects.
 *
 * @details
 * This class provides a concrete implementation of the CSourceFileModifier interface
 * using simple string appending operations. It modifies C header files by reading
 * existing content, appending new declarations, and writing the modified content
 * back to the file system.
 *
 * The implementation uses text-based file modification rather than AST manipulation,
 * making it simpler to implement and debug while being sufficient for the tileset
 * integration use case.
 *
 * Dependencies:
 * - ProjectPaths: For computing file paths within the pokeemerald project structure
 * - CSourceGenerator: For generating the C code constructs to append
 *
 * This class follows the established patterns in the Porytiles2 codebase:
 * - Uses Result for error handling
 * - Implements domain interface in infrastructure layer
 * - Uses dependency injection for services
 * - Provides detailed error messages for debugging
 */
class ProjectCSourceFileAppender final : public CSourceFileModifier {
  public:
    /**
     * @brief Constructs a ProjectCSourceFileAppender with required dependencies.
     *
     * @param paths Project path computation service (must not be null)
     * @param generator C source code generation service
     */
    explicit ProjectCSourceFileAppender(gsl::not_null<ProjectPaths *> paths,
                                        std::unique_ptr<CSourceGenerator> generator);

    [[nodiscard]] Result<void> append_to_graphics_header(const std::string &tileset_name) override;
    [[nodiscard]] Result<void> append_to_headers_header(const std::string &tileset_name) override;
    [[nodiscard]] Result<void> append_to_metatiles_header(const std::string &tileset_name) override;
    [[nodiscard]] Result<void> append_tileset_declarations(const std::string &tileset_name) override;

  private:
    const ProjectPaths *paths_;
    std::unique_ptr<CSourceGenerator> generator_;
};

} // namespace porytiles2
#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Loads behavior mappings from a C/C++ header file containing behavior definitions.
 *
 * @details
 * Parses header files in the formats used by Pokemon decomp projects. Supports two formats:
 *
 * Define format (older pokeemerald):
 * ```
 * #define MB_NORMAL 0x00
 * #define MB_TALL_GRASS 0x02
 * ```
 *
 * Enum format (newer pokeemerald):
 * ```
 * MB_NORMAL,
 * MB_TALL_GRASS, // optional comment
 * ```
 *
 * In the enum format, values are assigned sequentially starting from 0. Both formats support decimal and hexadecimal
 * values (for define format). The provider loads lazily on first lookup and caches the mappings. Entries named
 * `MB_INVALID` are skipped in both formats.
 */
class HeaderBehaviorMapProvider final : public BehaviorMapProvider {
  public:
    /**
     * @brief Constructs a provider that will load from the specified header file.
     *
     * @details
     * This constructor sets up the provider to read behavior mappings from the specified header file. The file may use
     * either the define format or the enum format. The header file is loaded lazily when first accessed via lookup()
     * and cached for subsequent lookups.
     *
     * @param project_root The root directory of the project
     * @param header_relative_path The path to the metatile_behaviors.h file relative to project_root
     * @param format The text formatter for styled output
     * @param diag The user diagnostics for error reporting
     */
    HeaderBehaviorMapProvider(
        const std::filesystem::path &project_root,
        const std::filesystem::path &header_relative_path,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag);

    [[nodiscard]] ChainableResult<std::uint16_t> lookup(const std::string &behavior_name) const override;

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint16_t behavior_value) const override;

  private:
    ChainableResult<void> ensure_loaded() const;

    std::filesystem::path project_root_;
    std::filesystem::path header_relative_path_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    std::unique_ptr<FileHighlightPrinter> file_printer_;
    mutable bool loaded_{false};
    mutable bool load_failed_{false};
    mutable std::vector<std::string> cached_lines_;
    mutable std::unordered_map<std::string, std::uint16_t> name_to_value_;
    mutable std::unordered_map<std::uint16_t, std::string> value_to_name_;
};

} // namespace porytiles2

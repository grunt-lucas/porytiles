#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "gsl/pointers"

#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/c_parser/source_position.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
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
        std::filesystem::path project_root,
        std::filesystem::path header_relative_path,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, header_relative_path_{std::move(header_relative_path)},
          format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<std::uint16_t> lookup(const std::string &behavior_name) const override;

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint16_t behavior_value) const override;

  private:
    ChainableResult<void> ensure_loaded() const;

    /**
     * @brief Attempts to add a behavior entry with duplicate detection and rich error reporting.
     *
     * @details
     * Uses duck typing to accept any entry type with name(), int_value(), and position() methods. Filters out entries
     * that don't match MB_* pattern or are MB_INVALID, validates value range, checks for duplicates, and inserts into
     * the maps if valid. On duplicate detection, produces rich error messages with source context showing both
     * locations.
     *
     * This template is defined in the .cpp file since it's only used internally with DefineStatement and EnumMember
     * types.
     *
     * @tparam Entry Type with name(), int_value(), and position() methods (e.g., DefineStatement, EnumMember)
     * @param entry The entry to add
     * @return Empty result on success (including filtered-out entries), error on duplicate
     * @note Despite being const, this method mutates mutable cache members (maps and load_failed_ flag)
     */
    template <typename Entry>
    ChainableResult<void> try_add_behavior_entry(const Entry &entry) const;

    std::filesystem::path project_root_;
    std::filesystem::path header_relative_path_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    mutable bool loaded_{false};
    mutable bool load_failed_{false};
    mutable std::unique_ptr<CParserFacade> driver_;
    mutable std::unordered_map<std::string, std::uint16_t> name_to_value_;
    mutable std::unordered_map<std::uint16_t, std::string> value_to_name_;
    mutable std::unordered_map<std::string, SourcePosition> name_to_position_;
    mutable std::unordered_map<std::uint16_t, SourcePosition> value_to_position_;
};

} // namespace porytiles2

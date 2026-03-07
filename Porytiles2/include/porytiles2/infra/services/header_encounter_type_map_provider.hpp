#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "gsl/pointers"

#include "porytiles2/domain/services/encounter_type_map_provider.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/c_parser/source_position.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Loads encounter type mappings from a C/C++ header file containing encounter definitions.
 *
 * @details
 * Parses header files in the formats used by Pokemon decomp projects. Supports two formats:
 *
 * Define format:
 * ```
 * #define TILE_ENCOUNTER_NONE 0x00
 * #define TILE_ENCOUNTER_LAND 0x01
 * ```
 *
 * Enum format:
 * ```
 * TILE_ENCOUNTER_NONE,
 * TILE_ENCOUNTER_LAND,
 * ```
 *
 * In the enum format, values are assigned sequentially starting from 0. Both formats support decimal and hexadecimal
 * values (for define format). The provider loads lazily on first lookup and caches the mappings. Encounter type values
 * occupy 3 bits (0-7) in the FireRed metatile attribute format.
 */
class HeaderEncounterTypeMapProvider final : public EncounterTypeMapProvider {
  public:
    /**
     * @brief Constructs a provider that will load from the specified header file.
     *
     * @details
     * This constructor sets up the provider to read encounter type mappings from the specified header file. The file
     * may use either the define format or the enum format. The header file is loaded lazily when first accessed via
     * lookup() and cached for subsequent lookups.
     *
     * @param header_path The path to the project metatile_behaviors.h file
     * @param format The text formatter for styled output
     * @param diag The user diagnostics for error reporting
     */
    HeaderEncounterTypeMapProvider(
        std::filesystem::path header_path,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : header_path_{std::move(header_path)}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<std::uint8_t> lookup(const std::string &encounter_name) const override;

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint8_t encounter_value) const override;

  private:
    ChainableResult<void> ensure_loaded() const;

    /**
     * @brief Attempts to add an encounter type entry with duplicate detection and rich error reporting.
     *
     * @details
     * Uses duck typing to accept any entry type with name(), int_value(), and position() methods. Filters out entries
     * that don't match TILE_ENCOUNTER_* pattern, validates value range (0-7), checks for duplicates, and inserts into
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
    ChainableResult<void> try_add_encounter_entry(const Entry &entry) const;

    std::filesystem::path header_path_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    mutable bool loaded_{false};
    mutable bool load_failed_{false};
    mutable std::unique_ptr<CParserFacade> driver_;
    mutable std::unordered_map<std::string, std::uint8_t> name_to_value_;
    mutable std::unordered_map<std::uint8_t, std::string> value_to_name_;
    mutable std::unordered_map<std::string, SourcePosition> name_to_position_;
    mutable std::unordered_map<std::uint8_t, SourcePosition> value_to_position_;
};

} // namespace porytiles2

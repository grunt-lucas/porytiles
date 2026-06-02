#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "gsl/pointers"

#include "porytiles/domain/services/terrain_type_map_provider.hpp"
#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/utilities/c_parser/source_position.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/**
 * @brief Loads terrain type mappings from a C/C++ header file containing terrain definitions.
 *
 * @details
 * Parses header files in the formats used by Pokemon decomp projects. Supports two formats:
 *
 * Define format:
 * ```
 * #define TILE_TERRAIN_NORMAL 0x00
 * #define TILE_TERRAIN_GRASS  0x01
 * ```
 *
 * Enum format:
 * ```
 * TILE_TERRAIN_NORMAL,
 * TILE_TERRAIN_GRASS,
 * ```
 *
 * In the enum format, values are assigned sequentially starting from 0. Both formats support decimal and hexadecimal
 * values (for define format). The provider loads lazily on first lookup and caches the mappings. Terrain type values
 * occupy 5 bits (0-31) in the FireRed metatile attribute format.
 */
class HeaderTerrainTypeMapProvider final : public TerrainTypeMapProvider {
  public:
    /**
     * @brief Constructs a provider that will load from the specified header file.
     *
     * @details
     * This constructor sets up the provider to read terrain type mappings from the specified header file. The file may
     * use either the define format or the enum format. The header file is loaded lazily when first accessed via
     * lookup() and cached for subsequent lookups.
     *
     * @param header_path The path to the project metatile_behaviors.h file
     * @param format The text formatter for styled output
     * @param diag The user diagnostics for error reporting
     */
    HeaderTerrainTypeMapProvider(
        std::filesystem::path header_path,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : header_path_{std::move(header_path)}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<std::uint8_t> lookup(const std::string &terrain_name) const override;

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint8_t terrain_value) const override;

  private:
    ChainableResult<void> ensure_loaded() const;

    /**
     * @brief Attempts to add a terrain type entry with duplicate detection and rich error reporting.
     *
     * @details
     * Uses duck typing to accept any entry type with name(), int_value(), and position() methods. Filters out entries
     * that don't match TILE_TERRAIN_* pattern, validates value range (0-31), checks for duplicates, and inserts into
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
    ChainableResult<void> try_add_terrain_entry(const Entry &entry) const;

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

} // namespace porytiles

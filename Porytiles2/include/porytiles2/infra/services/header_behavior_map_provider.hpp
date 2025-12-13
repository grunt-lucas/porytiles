#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/services/behavior_map_provider.hpp"

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
     * @param header_path Path to the metatile_behaviors.h file
     */
    explicit HeaderBehaviorMapProvider(std::filesystem::path header_path);

    [[nodiscard]] std::optional<std::uint16_t> lookup(const std::string &behavior_name) const override;

    [[nodiscard]] std::optional<std::string> lookup(std::uint16_t behavior_value) const override;

  private:
    void ensure_loaded() const;

    std::filesystem::path header_path_;
    mutable bool loaded_{false};
    mutable std::unordered_map<std::string, std::uint16_t> name_to_value_;
    mutable std::unordered_map<std::uint16_t, std::string> value_to_name_;
};

} // namespace porytiles2

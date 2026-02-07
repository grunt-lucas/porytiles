#pragma once

#include <cstdint>
#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for providing two-way metatile terrain type mappings.
 *
 * @details
 * The TerrainTypeMapProvider maps terrain type constant names (e.g., "TILE_TERRAIN_GRASS",
 * "TILE_TERRAIN_WATER") to their corresponding uint8_t values and vice versa. Implementations
 * may load these mappings from various sources such as header files or configuration. Loading
 * and caching strategies are implementation details left to concrete implementations.
 */
class TerrainTypeMapProvider {
  public:
    virtual ~TerrainTypeMapProvider() = default;

    /**
     * @brief Looks up the numeric value for a terrain type constant name.
     *
     * @details
     * This function searches for the given terrain type constant name in the provider's mapping
     * and returns its corresponding numeric value if found.
     *
     * @param terrain_name The terrain type constant name (e.g., "TILE_TERRAIN_GRASS")
     * @return The numeric value if found, or an error describing why the lookup failed
     */
    [[nodiscard]] virtual ChainableResult<std::uint8_t> lookup(const std::string &terrain_name) const = 0;

    /**
     * @brief Looks up the terrain type constant name for a numeric value.
     *
     * @details
     * This function performs a reverse lookup, searching for the terrain type constant name that
     * corresponds to the given numeric value.
     *
     * @param terrain_value The numeric terrain type value
     * @return The terrain type constant name if found, or an error describing why the lookup failed
     */
    [[nodiscard]] virtual ChainableResult<std::string> lookup(std::uint8_t terrain_value) const = 0;
};

} // namespace porytiles2

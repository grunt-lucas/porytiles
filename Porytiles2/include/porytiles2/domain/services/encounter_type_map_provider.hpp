#pragma once

#include <cstdint>
#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for providing two-way metatile encounter type mappings.
 *
 * @details
 * The EncounterTypeMapProvider maps encounter type constant names (e.g., "TILE_ENCOUNTER_NONE",
 * "TILE_ENCOUNTER_LAND") to their corresponding uint8_t values and vice versa. Implementations
 * may load these mappings from various sources such as header files or configuration. Loading
 * and caching strategies are implementation details left to concrete implementations.
 */
class EncounterTypeMapProvider {
  public:
    virtual ~EncounterTypeMapProvider() = default;

    /**
     * @brief Looks up the numeric value for an encounter type constant name.
     *
     * @details
     * This function searches for the given encounter type constant name in the provider's mapping
     * and returns its corresponding numeric value if found.
     *
     * @param encounter_name The encounter type constant name (e.g., "TILE_ENCOUNTER_NONE")
     * @return The numeric value if found, or an error describing why the lookup failed
     */
    [[nodiscard]] virtual ChainableResult<std::uint8_t> lookup(const std::string &encounter_name) const = 0;

    /**
     * @brief Looks up the encounter type constant name for a numeric value.
     *
     * @details
     * This function performs a reverse lookup, searching for the encounter type constant name that
     * corresponds to the given numeric value.
     *
     * @param encounter_value The numeric encounter type value
     * @return The encounter type constant name if found, or an error describing why the lookup failed
     */
    [[nodiscard]] virtual ChainableResult<std::string> lookup(std::uint8_t encounter_value) const = 0;
};

} // namespace porytiles2

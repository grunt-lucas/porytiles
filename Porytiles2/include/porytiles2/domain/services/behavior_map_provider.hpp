#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace porytiles2 {

/**
 * @brief Abstract interface for providing two-way metatile behavior mappings.
 *
 * @details
 * The BehaviorMapProvider maps behavior constant names (e.g., "MB_NORMAL", "MB_TALL_GRASS") to their corresponding
 * uint16_t values and vice versa. Implementations may load these mappings from various sources such as header files or
 * configuration. Loading and caching strategies are implementation details left to concrete implementations.
 */
class BehaviorMapProvider {
  public:
    virtual ~BehaviorMapProvider() = default;

    /**
     * @brief Looks up the numeric value for a behavior constant name.
     *
     * @details
     * This function searches for the given behavior constant name in the provider's mapping and returns its
     * corresponding numeric value if found.
     *
     * @param behavior_name The behavior constant name (e.g., "MB_NORMAL")
     * @return The numeric value if found, std::nullopt otherwise
     */
    [[nodiscard]] virtual std::optional<std::uint16_t> lookup(const std::string &behavior_name) const = 0;

    /**
     * @brief Looks up the behavior constant name for a numeric value.
     *
     * @details
     * This function performs a reverse lookup, searching for the behavior constant name that corresponds to the given
     * numeric value.
     *
     * @param behavior_value The numeric behavior value
     * @return The behavior constant name if found, std::nullopt otherwise
     */
    [[nodiscard]] virtual std::optional<std::string> lookup(std::uint16_t behavior_value) const = 0;
};

} // namespace porytiles2

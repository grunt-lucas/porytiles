#pragma once

#include <set>
#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for querying layout metadata by name.
 *
 * @details
 * LayoutMetadataProvider defines a domain-layer abstraction for retrieving metadata about layouts. This allows the
 * domain logic to query layout properties (such as height or width) without depending on any specific data source or
 * I/O mechanism. Concrete implementations may source this metadata from project files, configuration, or other external
 * systems.
 *
 * @invariant Implementations must return consistent results for the same layout_name_or_id within a single compilation
 * run.
 */
class LayoutMetadataProvider {
  public:
    virtual ~LayoutMetadataProvider() = default;

    /**
     * @brief Checks whether a layout exists in the backing store for the given layout name or ID.
     *
     * @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
     * "LAYOUT_PETALBURG_CITY")
     * @return True if the layout exists in the backing store, false otherwise
     */
    [[nodiscard]] virtual bool exists(const std::string &layout_name_or_id) const = 0;

    /**
     * @brief Determines the given layout's width in metatiles.
     *
     * @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
     * "LAYOUT_PETALBURG_CITY")
     * @return ChainableResult containing width of the layout
     */
    [[nodiscard]] virtual ChainableResult<std::size_t> width(const std::string &layout_name_or_id) const = 0;

    /**
     * @brief Determines the given layout's height in metatiles.
     *
     * @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
     * "LAYOUT_PETALBURG_CITY")
     * @return ChainableResult containing height of the layout
     */
    [[nodiscard]] virtual ChainableResult<std::size_t> height(const std::string &layout_name_or_id) const = 0;

    /**
     * @brief Determines the given layout's primary tileset.
     *
     * @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
     * "LAYOUT_PETALBURG_CITY")
     * @return ChainableResult containing primary tileset of the layout
     */
    [[nodiscard]] virtual ChainableResult<std::string> primary_tileset(const std::string &layout_name_or_id) const = 0;

    /**
     * @brief Determines the given layout's secondary tileset.
     *
     * @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
     * "LAYOUT_PETALBURG_CITY")
     * @return ChainableResult containing secondary tileset of the layout
     */
    [[nodiscard]] virtual ChainableResult<std::string>
    secondary_tileset(const std::string &layout_name_or_id) const = 0;

    /**
     * @brief Returns all layout names known to this provider.
     *
     * @details
     * Enumerates all layout names available in the backing store. This enables batch operations, validation, and
     * listing without requiring callers to know layout names in advance.
     *
     * @return ChainableResult containing a set of all layout names
     */
    [[nodiscard]] virtual ChainableResult<std::set<std::string>> layout_names() const = 0;

    /**
     * @brief Returns all layout IDs known to this provider.
     *
     * @details
     * Enumerates all layout IDs available in the backing store. This enables batch operations, validation, and listing
     * without requiring callers to know layout IDs in advance.
     *
     * @return ChainableResult containing a set of all layout IDs
     */
    [[nodiscard]] virtual ChainableResult<std::set<std::string>> layout_ids() const = 0;
};

} // namespace porytiles2

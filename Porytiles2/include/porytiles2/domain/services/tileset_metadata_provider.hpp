#pragma once

#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for querying tileset metadata by name.
 *
 * @details
 * TilesetMetadataProvider defines a domain-layer abstraction for retrieving metadata about tilesets. This allows the
 * domain logic to query tileset properties (such as whether a tileset is primary/secondary or has animations) without
 * depending on any specific data source or I/O mechanism. Concrete implementations may source this metadata from
 * project files, configuration, or other external systems.
 *
 * @invariant Implementations must return consistent results for the same tileset_name within a single compilation run.
 */
class TilesetMetadataProvider {
  public:
    virtual ~TilesetMetadataProvider() = default;

    /**
     * @brief Checks whether a tileset exists in the backing store for the given tileset name.
     *
     * @param tileset_name The name of the tileset to check (e.g., "gTileset_General")
     * @return True if the tileset exists in the backing store, false otherwise
     */
    [[nodiscard]] virtual bool exists(const std::string &tileset_name) const = 0;

    /**
     * @brief Determines whether a tileset is a secondary tileset.
     *
     * @details
     * In pokeemerald, tilesets are either primary or secondary. Primary tilesets contain shared tiles and palettes,
     * while secondary tilesets contain map-specific content and layer on top of primaries. This distinction affects
     * tile indexing, palette allocation, and metatile attribute encoding.
     *
     * @param tileset_name The identifier for the tileset (e.g., "gTileset_General")
     * @return ChainableResult containing true if the tileset is secondary, false if primary
     */
    [[nodiscard]] virtual ChainableResult<bool> is_secondary(const std::string &tileset_name) const = 0;

    /**
     * @brief Determines whether a tileset has animation support.
     *
     * @details
     * Animated tilesets have a non-null animation callback function that cycles through animation frames. This
     * information is needed to properly handle tile references that may animate at runtime.
     *
     * @param tileset_name The identifier for the tileset (e.g., "gTileset_General")
     * @return ChainableResult containing true if the tileset has animations, false otherwise
     */
    [[nodiscard]] virtual ChainableResult<bool> has_animations(const std::string &tileset_name) const = 0;
};

} // namespace porytiles2

#pragma once

#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for querying Porytiles tileset ownership.
 *
 * @details
 * This interface provides a domain-layer abstraction for determining whether a tileset is managed by Porytiles. A
 * Porytiles-managed tileset is one that was either imported into or created by Porytiles, as opposed to a vanilla
 * pokeemerald tileset that has not been processed.
 *
 * Implementations may use different strategies to determine ownership (e.g., checking for marker files, querying a
 * database, etc.).
 *
 * @see ProjectPorytilesTilesetManager for the pokeemerald project-based implementation
 */
class PorytilesTilesetManager {
  public:
    virtual ~PorytilesTilesetManager() = default;

    /**
     * @brief Checks whether a tileset is managed by Porytiles.
     *
     * @details
     * If the tileset does not exist, this function returns false.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @return true if the tileset is Porytiles-managed
     */
    [[nodiscard]] virtual bool is_porytiles_managed(const std::string &tileset_name) const = 0;

    /**
     * @brief Persists managed state for an existing tileset.
     *
     * @details
     * Used when an existing tileset in headers.h is being converted to Porytiles-managed. The implementation reads
     * original field values from headers.h and stores them in the tileset manifest for potential restoration later.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @pre tileset_name must correspond to an existing tileset in headers.h
     * @return a ChainableResult indicating success or containing error details
     */
    [[nodiscard]] virtual ChainableResult<void> persist_managed_existing(const std::string &tileset_name) const = 0;

    /**
     * @brief Persists managed state for a new tileset.
     *
     * @details
     * Used when a new tileset is being added from scratch. The implementation creates a new entry in headers.h rather
     * than modifying an existing one.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_MyNewTileset")
     * @pre tileset_name must not already exist in headers.h
     * @return a ChainableResult indicating success or containing error details
     */
    [[nodiscard]] virtual ChainableResult<void> persist_managed_new(const std::string &tileset_name) const = 0;
};

} // namespace porytiles2

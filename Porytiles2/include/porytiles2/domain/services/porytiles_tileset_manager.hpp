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
     * @brief Persists managed state for the given tileset.
     *
     * @details
     * The specific details of tileset state management are implementation-defined.
     *
     * @param tileset_name The name of the tileset (e.g., "gTileset_General")
     * @return a ChainableResult indicating success or containing error details
     */
    [[nodiscard]] virtual ChainableResult<void> persist_managed_state(const std::string &tileset_name) const = 0;
};

} // namespace porytiles2

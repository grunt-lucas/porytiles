#pragma once

#include <string>

#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Abstract interface for querying Porytiles tileset ownership.
///
/// @details
/// This interface provides a domain-layer abstraction for determining whether a tileset is managed by Porytiles. A
/// Porytiles-managed tileset is one that was either imported into or created by Porytiles, as opposed to a vanilla
/// pokeemerald tileset that has not been processed.
///
/// Implementations may use different strategies to determine ownership (e.g., checking for marker files, querying a
/// database, etc.).
///
/// @see ProjectPorytilesTilesetManager for the pokeemerald project-based implementation
class PorytilesTilesetManager {
  public:
    virtual ~PorytilesTilesetManager() = default;

    /// @brief Checks whether a tileset is managed by Porytiles.
    ///
    /// @details
    /// If the tileset does not exist, this function returns false.
    ///
    /// @param tileset_name The name of the tileset (e.g., "gTileset_General")
    /// @return true if the tileset is Porytiles-managed
    [[nodiscard]] virtual bool is_porytiles_managed(const std::string &tileset_name) const = 0;

    /// @brief Persists managed state for an existing tileset.
    ///
    /// @details
    /// Used when an existing tileset in headers.h is being converted to Porytiles-managed. The implementation reads
    /// original field values from headers.h and stores them in the tileset manifest for potential restoration later.
    ///
    /// @param tileset_name The name of the tileset (e.g., "gTileset_General")
    /// @pre tileset_name must correspond to an existing tileset in headers.h
    /// @return a ChainableResult indicating success or containing error details
    [[nodiscard]] virtual ChainableResult<void> persist_managed_existing(const std::string &tileset_name) const = 0;

    /// @brief Persists managed state for a new tileset.
    ///
    /// @details
    /// Used when a new tileset is being added from scratch. The implementation creates a new entry in headers.h rather
    /// than modifying an existing one.
    ///
    /// @param tileset_name The name of the tileset (e.g., "gTileset_MyNewTileset")
    /// @pre tileset_name must not already exist in headers.h
    /// @return a ChainableResult indicating success or containing error details
    [[nodiscard]] virtual ChainableResult<void>
    persist_managed_new(const std::string &tileset_name, bool is_secondary = false) const = 0;

    /// @brief Wires animation code for a tileset that already has its manifest persisted.
    ///
    /// @details
    /// Called after compilation when a tileset has animations. Performs:
    /// 1. Adds #include for generated_anim_code.h in tileset_anims.c
    /// 2. Adds function declaration in tileset_anims.h
    /// 3. Updates .callback field in headers.h
    ///
    /// Idempotent - safe to call multiple times.
    ///
    /// @param tileset_name The tileset name (e.g., "gTileset_MyTileset")
    /// @param is_secondary True for secondary tilesets, false for primary
    /// @pre tileset_name must be an existing Porytiles-managed tileset
    /// @return ChainableResult indicating success or error details
    [[nodiscard]] virtual ChainableResult<void>
    wire_anim_code(const std::string &tileset_name, bool is_secondary) const = 0;

    /// @brief Removes wired animation code for a tileset from the project.
    ///
    /// @details
    /// Performs the inverse of wire_anim_code(). This method:
    /// 1. Removes the #include directive from tileset_anims.c
    /// 2. Removes the function declaration from tileset_anims.h
    /// 3. Updates the .callback field in headers.h to "NULL" (only if it's a Porytiles-managed callback)
    ///
    /// The callback is only cleared if it starts with "InitTilesetAnim_PorytilesManaged_". User-managed callbacks are
    /// preserved, allowing users to set wire_anim_code=false while maintaining their own custom animation callbacks.
    ///
    /// This method is idempotent - safe to call even if no wiring exists. Should be called after compilation when a
    /// tileset has no animations to ensure stale animation references are cleaned up.
    ///
    /// @param tileset_name Name of the tileset to remove wiring for
    /// @param is_secondary True if this is a secondary tileset
    /// @return ChainableResult<void> Success or error
    [[nodiscard]] virtual ChainableResult<void>
    remove_wired_anim_code(const std::string &tileset_name, bool is_secondary) const = 0;
};

} // namespace porytiles

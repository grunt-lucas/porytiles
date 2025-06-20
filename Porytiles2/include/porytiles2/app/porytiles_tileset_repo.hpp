#pragma once

#include <expected>
#include <memory>
#include <string>

#include <porytiles2/domain/tilesets/porytiles_tileset.hpp>

namespace porytiles {

/**
 * @brief Repository interface for the PorytilesTileset aggregate root.
 *
 * @details
 * The PorytilesTilesetRepo makes no assumptions about the structure of the backing store for the PorytilesTileset.
 * Presumably, this store is the canonical 'data/tilesets' directory, but the details here are implementation-defined.
 */
class PorytilesTilesetRepo {
  public:
    virtual ~PorytilesTilesetRepo() = default;

    /**
     * @brief Persists a new or existing PorytilesTileset.
     *
     * @param tileset The PorytilesTileset aggregate to save.
     */
    virtual void save(const PorytilesTileset &tileset) = 0;

    /**
     * @brief Loads an existing PorytilesTileset from storage.
     *
     * @param name The name of the PorytilesTileset aggregate to load.
     * @return A `unique_ptr` to the loaded PorytilesTileset, or a string describing the load error.
     */
    virtual std::expected<std::unique_ptr<PorytilesTileset>, std::string> load(const std::string &name) = 0;
};

} // namespace porytiles

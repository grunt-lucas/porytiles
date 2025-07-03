#pragma once

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/PorymapTileset.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Repository interface for the PorymapTileset aggregate root.
 *
 * @details
 * The PorymapTilesetRepo makes no assumptions about the structure of the
 * backing store for the PorymapTileset. Presumably, this store is the canonical
 * 'data/tilesets' directory, but the details here are implementation-defined.
 */
class PorymapTilesetRepo {
public:
  virtual ~PorymapTilesetRepo() = default;

  /**
   * @brief Persists a new or existing PorymapTileset.
   *
   * @param tileset The PorymapTileset aggregate to save.
   * @return An empty Result on success, otherwise an error description.
   */
  virtual Result<void> Save(const PorymapTileset &tileset) = 0;

  /**
   * @brief Loads an existing PorymapTileset from storage.
   *
   * @param name The name of the PorymapTileset aggregate to load.
   * @return A PorymapTileset Result on success, otherwise an error description.
   */
  virtual Result<std::unique_ptr<PorymapTileset>>
  Load(const std::string &name) = 0;
};

} // namespace porytiles

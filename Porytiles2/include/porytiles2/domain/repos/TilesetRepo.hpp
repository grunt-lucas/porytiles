#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Repository interface for the Tileset aggregate root.
 *
 * @details
 * The TilesetRepo makes no assumptions about the structure of the backing store
 * for the Tileset. Presumably, this store is the canonical 'data/tilesets'
 * directory, but the details here are implementation-defined.
 */
class TilesetRepo {
public:
  virtual ~TilesetRepo() = default;

  /**
   * @brief Persists a new or existing Tileset.
   *
   * @param tileset The Tileset aggregate to save.
   * @return An empty Result on success, otherwise an error description.
   */
  virtual Result<void> Save(const Tileset &tileset) = 0;

  /**
   * @brief Loads an existing Tileset from storage.
   *
   * @param name The name of the Tileset aggregate to load.
   * @return A Tileset Result on success, otherwise an error description.
   */
  virtual Result<std::unique_ptr<Tileset>> Load(const std::string &name) = 0;
};

} // namespace porytiles

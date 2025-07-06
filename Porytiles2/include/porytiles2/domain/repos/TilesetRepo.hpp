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

  /*
   * TODO : Partially implement save: we should force implementers to persis updated artifact
   * checksums. To that end, we can have Save be partially implemented, but then call a virtual
   * SaveTileset that can be overridden by the implementer.
   *
   * How should we handle the artifact checksums? They seem to be a detail more related to data
   * persistence than the Tileset itself. Is there some way to design things so that we can support
   * the outlined use cases while avoiding having to store checksums in the Tileset aggregate?
   */

  /**
   * @brief Persists a new or existing Tileset.
   *
   * @details
   * When persisting a Tileset, the repository
   *
   * @param tileset The Tileset to save.
   * @return An empty Result on success, otherwise an error description.
   */
  [[nodiscard]] virtual Result<void> Save(const Tileset &tileset) = 0;

  /**
   * @brief Loads an existing Tileset from storage.
   *
   * @param name The name of the Tileset to load.
   * @return A Tileset Result on success, otherwise an error description.
   */
  [[nodiscard]] virtual Result<std::unique_ptr<Tileset>> Load(const std::string &name) = 0;

  /**
   * @brief Checks if the given Tileset exists in the backing store.
   *
   * @param name The name of the Tileset to check.
   * @return True if the named tileset exists, false otherwise.
   */
  [[nodiscard]] virtual bool Exists(const std::string &name) const = 0;

  /**
   * @brief Computes checksums for the artifacts that correspond to this Tileset's
   * PorymapTilesetComponent.
   *
   * @details
   *
   *
   * @param name The name of the Tileset for which to compute checksums.
   * @return A mapping of artifact identifiers to their computed checksum.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  ComputePorymapChecksums(const std::string &name) const = 0;
};

} // namespace porytiles

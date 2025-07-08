#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/domain/services/ArtifactMetadataProvider.hpp"
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

  explicit TilesetRepo(std::unique_ptr<ArtifactMetadataProvider> checksum_service)
      : metadata_service_{std::move(checksum_service)} {}

  /**
   * @brief Persists a given Tileset and computes new artifact checksums.
   *
   * @details
   * When persisting a Tileset, the repository saves the tileset and computes
   * new artifact checksums for the persisted data.
   *
   * @param tileset The Tileset to save.
   * @return An empty Result on success, otherwise an error description.
   */
  [[nodiscard]] Result<void> Save(const Tileset &tileset) {
    if (auto save_result = SaveTileset(tileset); !save_result.has_value()) {
      return save_result;
    }

    const auto current_checksums = metadata_service_->ComputePorymapChecksums(tileset);
    return metadata_service_->StoreChecksums(tileset.name(), current_checksums);
  }

  /**
   * @brief Loads an existing Tileset from storage.
   *
   * @param name The name of the Tileset to load.
   * @return A Tileset Result on success, otherwise an error description.
   */
  [[nodiscard]] virtual Result<std::unique_ptr<Tileset>> Load(const std::string &name) const = 0;

  /**
   * @brief Checks if the given Tileset exists in the backing store.
   *
   * @param name The name of the Tileset to check.
   * @return True if the named tileset exists, false otherwise.
   */
  [[nodiscard]] virtual bool Exists(const std::string &name) const = 0;

protected:
  /**
   * @brief Persists a given Tileset.
   *
   * @param tileset The Tileset to save.
   * @return An empty Result on success, otherwise an error description.
   *
   */
  [[nodiscard]] virtual Result<void> SaveTileset(const Tileset &tileset) = 0;

  [[nodiscard]] ArtifactMetadataProvider &metadata_service() { return *metadata_service_; }

  [[nodiscard]] const ArtifactMetadataProvider &metadata_service() const {
    return *metadata_service_;
  }

private:
  std::unique_ptr<ArtifactMetadataProvider> metadata_service_;
};

} // namespace porytiles

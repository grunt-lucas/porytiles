#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/tileset.hpp"
#include "porytiles2/domain/services/artifact_metadata_provider.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Repository interface for the Tileset aggregate root.
 *
 * @details
 * The TilesetRepo makes no assumptions about the structure of the backing store for the Tileset. Presumably, this store
 * is the canonical 'data/tilesets' directory, but the details here are implementation-defined.
 */
class TilesetRepo {
  public:
    virtual ~TilesetRepo() = default;

    explicit TilesetRepo(std::unique_ptr<ArtifactMetadataProvider> metadata_provider)
        : metadata_provider_{std::move(metadata_provider)} {}

    /**
     * @brief Persists a given Tileset and computes new artifact checksums.
     *
     * @details
     * When persisting a Tileset, the repository saves the tileset and computes new artifact checksums for the persisted
     * data.
     *
     * @param tileset The Tileset to save.
     * @return An empty Result on success, otherwise an error description.
     */
    [[nodiscard]] Result<void> save(const Tileset &tileset) {
        if (auto save_result = save_tileset(tileset); !save_result.has_value()) {
            return save_result;
        }

        const auto current_checksums = metadata_provider_->compute_porymap_checksums(tileset);
        return metadata_provider_->store_checksums(tileset.name(), current_checksums);
    }

    /**
     * @brief Loads an existing Tileset from storage.
     *
     * @param name The name of the Tileset to load.
     * @return A Tileset Result on success, otherwise an error description.
     */
    [[nodiscard]] virtual Result<std::unique_ptr<Tileset>> load(const std::string &name) const = 0;

    /**
     * @brief Checks if the given Tileset exists in the backing store.
     *
     * @param name The name of the Tileset to check.
     * @return True if the named tileset exists, false otherwise.
     */
    [[nodiscard]] virtual bool exists(const std::string &name) const = 0;

  protected:
    /**
     * @brief Persists a given Tileset.
     *
     * @param tileset The Tileset to save.
     * @return An empty Result on success, otherwise an error description.
     *
     */
    [[nodiscard]] virtual Result<void> save_tileset(const Tileset &tileset) = 0;

    [[nodiscard]] ArtifactMetadataProvider &metadata_service() {
        return *metadata_provider_;
    }

    [[nodiscard]] const ArtifactMetadataProvider &metadata_service() const {
        return *metadata_provider_;
    }

  private:
    std::unique_ptr<ArtifactMetadataProvider> metadata_provider_;
};

} // namespace porytiles2

#pragma once

#include <memory>
#include <string>

#include "gsl/pointers"

#include "../services/artifact_checksum_provider.hpp"
#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/domain/repos/tileset_artifact_writer.hpp"
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

    /**
     * @brief Constructs a TilesetRepo with the required dependencies.
     *
     * @details
     * Initializes the repository with all necessary components for tileset persistence operations. These dependencies
     * provide the concrete implementations for metadata management, key generation, and artifact I/O operations.
     *
     * @param checksum_provider Provider for computing and caching artifact checksums
     * @param key_provider Provider for generating keys and discovering artifacts in the backing store
     * @param reader Reader implementation for loading artifacts from the backing store
     * @param writer Writer implementation for saving artifacts to the backing store
     */
    explicit TilesetRepo(
        gsl::not_null<ArtifactChecksumProvider *> checksum_provider,
        gsl::not_null<TilesetArtifactKeyProvider *> key_provider,
        gsl::not_null<TilesetArtifactReader *> reader,
        gsl::not_null<TilesetArtifactWriter *> writer)
        : checksum_provider_{checksum_provider}, key_provider_{key_provider}, reader_{reader}, writer_{writer} {}

    /**
     * @brief Persists a given Tileset and caches new artifact checksums.
     *
     * @details
     * When persisting a Tileset, the repository saves the tileset and caches new artifact checksums for the persisted
     * data.
     *
     * @param tileset The Tileset to save.
     * @return An empty Result on success, otherwise an error description.
     */
    [[nodiscard]] Result<void> save(const Tileset &tileset) const;

    /**
     * @brief Loads an existing Tileset from storage.
     *
     * @param name The name of the Tileset to load.
     * @return A Tileset Result on success, otherwise an error description.
     */
    [[nodiscard]] Result<std::unique_ptr<Tileset>> load(const std::string &name) const;

    /**
     * @brief Checks if the given Tileset exists in the backing store.
     *
     * @param name The name of the Tileset to check.
     * @return True if the named tileset exists, false otherwise.
     */
    [[nodiscard]] virtual bool exists(const std::string &name) const = 0;

    /**
     * @brief Gets a reference to the ArtifactChecksumProvider for this repo.
     *
     * @return Reference to the provider
     */
    [[nodiscard]] ArtifactChecksumProvider &checksum_provider() const {
        return *checksum_provider_;
    }

    /**
     * @brief Gets a reference to the TilesetArtifactKeyProvider for this repo.
     *
     * @return Reference to the provider
     */
    [[nodiscard]] TilesetArtifactKeyProvider &key_provider() const {
        return *key_provider_;
    }

  private:
    ArtifactChecksumProvider *checksum_provider_;
    TilesetArtifactKeyProvider *key_provider_;
    TilesetArtifactReader *reader_;
    TilesetArtifactWriter *writer_;
};

} // namespace porytiles2

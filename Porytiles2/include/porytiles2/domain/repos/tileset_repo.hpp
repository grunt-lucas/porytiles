#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/domain/repos/tileset_artifact_writer.hpp"
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

    /**
     * @brief Constructs a TilesetRepo with the required dependencies.
     *
     * @details
     * Initializes the repository with all necessary components for tileset persistence operations. These dependencies
     * provide the concrete implementations for metadata management, key generation, and artifact I/O operations.
     *
     * @param metadata_provider Provider for computing and caching artifact checksums
     * @param key_provider Provider for generating keys and discovering artifacts in the backing store
     * @param reader Reader implementation for loading artifacts from the backing store
     * @param writer Writer implementation for saving artifacts to the backing store
     */
    explicit TilesetRepo(std::unique_ptr<ArtifactMetadataProvider> metadata_provider,
                         std::unique_ptr<TilesetArtifactKeyProvider> key_provider,
                         std::unique_ptr<TilesetArtifactReader> reader, std::unique_ptr<TilesetArtifactWriter> writer)
        : metadata_provider_{std::move(metadata_provider)}, key_provider_{std::move(key_provider)},
          reader_{std::move(reader)}, writer_{std::move(writer)} {}

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
     * @brief Gets a reference to the metadata provider.
     *
     * @details
     * Provides access to the metadata provider for computing artifact checksums and managing artifact caches. This is
     * primarily used by derived implementations for checksum validation and caching operations.
     *
     * @return Reference to the artifact metadata provider
     */
    [[nodiscard]] ArtifactMetadataProvider &metadata_provider() const {
        return *metadata_provider_;
    }

  protected:
    /**
     * @brief Creates an empty tileset instance for loading.
     *
     * @details
     * Load calls this factory method to create the appropriate tileset type.
     * Derived classes should override this to create their specific tileset implementation.
     *
     * @return A unique pointer to a new empty Tileset instance.
     */
    [[nodiscard]] virtual std::unique_ptr<Tileset> create_empty_tileset() const = 0;

  private:
    std::unique_ptr<ArtifactMetadataProvider> metadata_provider_;
    std::unique_ptr<TilesetArtifactKeyProvider> key_provider_;
    std::unique_ptr<TilesetArtifactReader> reader_;
    std::unique_ptr<TilesetArtifactWriter> writer_;
};

} // namespace porytiles2

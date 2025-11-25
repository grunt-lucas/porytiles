#pragma once

#include <string>
#include <unordered_map>

#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract service for managing artifact checksums.
 *
 * @details
 * The ArtifactChecksumProvider provides an interface for accessing and managing metadata about both Porymap and
 * Porytiles artifacts. This includes computing and storing checksums for integrity verification, retrieving
 * modification timestamps, and determining temporal relationships between different artifact sets.
 *
 * Among other things, this service is essential for the compilation pipeline to determine when assets need to be
 * recompiled based on changes to source files or existing artifacts.
 */
class ArtifactChecksumProvider {
  public:
    virtual ~ArtifactChecksumProvider() = default;

    /**
     * @brief Computes checksums for the artifacts that belong to the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to compute checksums
     * @return A mapping of artifact keys to their computed checksum
     */
    [[nodiscard]] virtual std::unordered_map<ArtifactKey, std::string>
    compute_tileset_artifact_checksums(const std::string &tileset_name) const = 0;

    /**
     * @brief Loads the cached checksums for the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to load cached checksums
     * @return A mapping of artifact keys to their cached checksums
     */
    [[nodiscard]] virtual std::unordered_map<ArtifactKey, std::string>
    load_cached_tileset_checksums(const std::string &tileset_name) const = 0;

    /**
     * @brief Caches checksums for the given Tileset to persistent storage.
     *
     * @param tileset_name The name of the Tileset for which to cache checksums
     * @param checksums A mapping of artifact keys to their checksums to be cached
     * @return ChainableResult indicating success or failure of the cache operation
     */
    [[nodiscard]] virtual ChainableResult<void> cache_tileset_checksums(
        const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const = 0;

    /**
     * @brief Finds all artifacts for the given Tileset with unsynced changes compared to cached checksums.
     *
     * @details
     * This method compares current checksums against cached checksums for the specified artifact keys and returns a
     * vector of keys that don't match.
     *
     * @param tileset_name The name of the tileset to check
     * @param artifact_keys The keys of artifacts to check
     * @return Vector of artifact keys that have mismatched checksums
     */
    [[nodiscard]] std::vector<ArtifactKey> find_unsynced_tileset_artifacts(
        const std::string &tileset_name, const std::vector<ArtifactKey> &artifact_keys) const;

    /**
     * @brief Check if any cached checksums exist for the given tileset.
     *
     * @param tileset_name The name of the tileset to check
     * @return If any cached checksums exist for the given tileset
     */
    [[nodiscard]] bool cached_checksums_exist(const std::string &tileset_name) const;

    /**
     * @brief Checks if all artifact checksums for the given Tileset match their cached values.
     *
     * @details
     * This method compares current checksums against cached checksums for the specified artifact keys and returns true
     * if all match. It's effectively a convenience wrapper around find_unsynced_artifacts that simply checks if there
     * are no unsynced artifacts.
     *
     * @param tileset_name The name of the tileset to check
     * @param artifact_keys The keys of artifacts to check
     * @return True if all checksums match, false if any differ
     */
    [[nodiscard]] bool
    all_checksums_tileset_match(const std::string &tileset_name, const std::vector<ArtifactKey> &artifact_keys) const;
};

} // namespace porytiles2

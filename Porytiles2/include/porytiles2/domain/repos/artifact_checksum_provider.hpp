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
 * The ArtifactChecksumProvider provides an interface for accessing and managing checksums for both Porymap and
 * Porytiles artifacts.
 *
 * Among other things, this service is essential for the compilation pipeline to determine when assets need to be
 * recompiled based on changes to source files or existing artifacts.
 */
class ArtifactChecksumProvider {
  public:
    virtual ~ArtifactChecksumProvider() = default;

    /**
     * @brief Computes checksums for the given Tileset artifacts.
     *
     * @param keys The artifact keys for which to compute checksums
     * @return A mapping of artifact keys to their computed checksum
     */
    [[nodiscard]] virtual std::unordered_map<ArtifactKey, std::string>
    compute_tileset_artifact_checksums(const std::vector<ArtifactKey> &keys) const = 0;

    /**
     * @brief Loads the cached checksums for the given Tileset.
     *
     * @param name The name of the Tileset for which to load cached checksums
     * @return A mapping of artifact keys to their cached checksums
     */
    [[nodiscard]] virtual std::unordered_map<ArtifactKey, std::string>
    load_cached_tileset_checksums(const std::string &name) const = 0;

    /**
     * @brief Caches checksums for the given Tileset to persistent storage.
     *
     * @param name The name of the Tileset for which to cache checksums
     * @param checksums A mapping of artifact keys to their checksums to be cached
     * @return ChainableResult indicating success or failure of the cache operation
     */
    [[nodiscard]] virtual ChainableResult<void> cache_tileset_checksums(
        const std::string &name, const std::unordered_map<ArtifactKey, std::string> &checksums) const = 0;

    /**
     * @brief Finds all artifacts for the given Tileset with unsynced changes compared to cached checksums.
     *
     * @details
     * This method compares current checksums against cached checksums for the specified artifact keys and returns a
     * vector of keys that don't match.
     *
     * @param name The name of the tileset to check
     * @param keys_to_check The keys of artifacts to check
     * @return Vector of artifact keys that have mismatched checksums
     */
    [[nodiscard]] std::vector<ArtifactKey>
    find_unsynced_tileset_artifacts(const std::string &name, const std::vector<ArtifactKey> &keys_to_check) const
    {
        const auto computed_checksums = compute_tileset_artifact_checksums(keys_to_check);
        const auto cached_checksums = load_cached_tileset_checksums(name);

        if (cached_checksums.empty()) {
            return {};
        }

        std::vector<ArtifactKey> mismatched_keys;
        for (const auto &key : keys_to_check) {
            const auto computed_checksum_for_key = computed_checksums.contains(key) ? computed_checksums.at(key) : "";
            const auto cached_checksum_for_key = cached_checksums.contains(key) ? cached_checksums.at(key) : "";
            if (computed_checksum_for_key != cached_checksum_for_key) {
                mismatched_keys.push_back(key);
            }
        }
        return mismatched_keys;
    }

    /**
     * @brief Check if any cached checksums exist for the given tileset.
     *
     * @param name The name of the tileset to check
     * @return If any cached checksums exist for the given tileset
     */
    [[nodiscard]] bool cached_checksums_exist(const std::string &name) const
    {
        return !load_cached_tileset_checksums(name).empty();
    }

    /**
     * @brief Checks if all artifact checksums for the given Tileset match their cached values.
     *
     * @details
     * This method compares current checksums against cached checksums for the specified artifact keys and returns true
     * if all match. It's effectively a convenience wrapper around find_unsynced_artifacts that simply checks if there
     * are no unsynced artifacts.
     *
     * @param name The name of the tileset to check
     * @param artifact_keys The keys of artifacts to check
     * @return True if all checksums match, false if any differ
     */
    [[nodiscard]] bool
    all_checksums_tileset_match(const std::string &name, const std::vector<ArtifactKey> &artifact_keys) const
    {
        return find_unsynced_tileset_artifacts(name, artifact_keys).empty();
    }
};

} // namespace porytiles2

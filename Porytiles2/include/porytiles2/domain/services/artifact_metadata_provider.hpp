#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "fmt/format.h"

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/templates/result.hpp"

#include <ranges>

namespace porytiles2 {

using Timestamp = std::filesystem::file_time_type;

/**
 * @brief Abstract service for managing artifact metadata including checksums and timestamps.
 *
 * @details
 * The ArtifactMetadataProvider provides an interface for accessing and managing metadata about both Porymap and
 * Porytiles artifacts. This includes computing and storing checksums for integrity verification, retrieving
 * modification timestamps, and determining temporal relationships between different artifact sets.
 *
 * This service is essential for the compilation pipeline to determine when assets need
 * to be recompiled based on changes to source files or existing artifacts.
 */
class ArtifactMetadataProvider {
  public:
    virtual ~ArtifactMetadataProvider() = default;

    /**
     * @brief Gets all keys for Porytiles artifacts in the given Tileset.
     *
     * @details
     * Each Porytiles artifact must have a unique key by which the ArtifactMetadataProvider and the TilesetRepo can
     * identify it.
     *
     * @return A vector of Porytiles artifact keys for the given Tileset
     */
    [[nodiscard]] virtual std::vector<std::string>
    get_porytiles_artifact_keys(const std::string &tileset_name) const = 0;

    /**
     * @brief Gets all keys for Porymap artifacts in the given Tileset.
     *
     * @details
     * Each Porymap artifact must have a unique key by which the ArtifactMetadataProvider and the TilesetRepo can
     * identify it.
     *
     * @return A vector of Porymap artifact keys for the given Tileset
     */
    [[nodiscard]] virtual std::vector<std::string> get_porymap_artifact_keys(const std::string &tileset_name) const = 0;

    /**
     * @brief Computes checksums for the artifacts that belong to the given Tileset.
     *
     * @param tileset_name The name of the tileset for which to compute checksums
     * @return A mapping of artifact identifiers to their computed checksum
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    compute_artifact_checksums(const std::string &tileset_name) const = 0;

    /**
     * @brief Loads the cached checksums for the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to load cached checksums
     * @return A mapping of artifact identifiers to their cached checksums
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    load_cached_checksums(const std::string &tileset_name) const = 0;

    /**
     * @brief Caches checksums for the given Tileset to persistent storage.
     *
     * @param tileset_name The name of the Tileset for which to cache checksums
     * @param checksums A mapping of artifact identifiers to their checksums to be cached
     * @return Result indicating success or failure of the cache operation
     */
    [[nodiscard]] virtual Result<void>
    cache_checksums(const std::string &tileset_name,
                    const std::unordered_map<std::string, std::string> &checksums) const = 0;

    /**
     * @brief Gets the modification timestamps for all Porymap artifacts associated with the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to get timestamps
     * @return A mapping of artifact identifiers to their modification timestamps
     */
    [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
    get_porymap_timestamps(const std::string &tileset_name) const = 0;

    /**
     * @brief Gets the modification timestamps for all Porytiles artifacts associated with the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to get timestamps
     * @return A mapping of artifact identifiers to their modification timestamps
     */
    [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
    get_porytiles_timestamps(const std::string &tileset_name) const = 0;

    /**
     * @brief Determines if the oldest Porytiles asset is newer than the newest Porymap asset.
     *
     * @param tileset_name The name of the Tileset for which to check asset modification times
     * @return True if Porytiles assets are newer, false otherwise
     */
    [[nodiscard]] virtual bool are_porytiles_assets_newer(const std::string &tileset_name) const {
        const auto porymap_timestamps = get_porymap_timestamps(tileset_name);
        const auto porytiles_timestamps = get_porytiles_timestamps(tileset_name);

        if (porytiles_timestamps.empty() || porymap_timestamps.empty()) {
            return false;
        }

        // Find the oldest Porytiles timestamp
        auto oldest_porytiles = porytiles_timestamps.begin()->second;
        for (const auto &timestamp : porytiles_timestamps | std::views::values) {
            if (timestamp < oldest_porytiles) {
                oldest_porytiles = timestamp;
            }
        }

        // Find the newest Porymap timestamp
        auto newest_porymap = porymap_timestamps.begin()->second;
        for (const auto &timestamp : porymap_timestamps | std::views::values) {
            if (timestamp > newest_porymap) {
                newest_porymap = timestamp;
            }
        }

        return oldest_porytiles > newest_porymap;
    }

    /**
     * @brief Determines if the oldest Porymap asset is newer than the newest Porytiles asset.
     *
     * @param tileset_name The name of the Tileset for which to check asset modification times
     * @return True if Porymap assets are newer, false otherwise
     */
    [[nodiscard]] virtual bool are_porymap_assets_newer(const std::string &tileset_name) const {
        const auto porymap_timestamps = get_porymap_timestamps(tileset_name);
        const auto porytiles_timestamps = get_porytiles_timestamps(tileset_name);

        if (porymap_timestamps.empty() || porytiles_timestamps.empty()) {
            return false;
        }

        // Find the oldest Porymap timestamp
        auto oldest_porymap = porymap_timestamps.begin()->second;
        for (const auto &timestamp : porymap_timestamps | std::views::values) {
            if (timestamp < oldest_porymap) {
                oldest_porymap = timestamp;
            }
        }

        // Find the newest Porytiles timestamp
        auto newest_porytiles = porytiles_timestamps.begin()->second;
        for (const auto &timestamp : porytiles_timestamps | std::views::values) {
            if (timestamp > newest_porytiles) {
                newest_porytiles = timestamp;
            }
        }

        return oldest_porymap > newest_porytiles;
    }

    /**
     * @brief Finds all artifacts with unsynced changes compared to cached checksums.
     *
     * @details
     * This method compares current checksums against cached checksums for the specified artifact keys and returns a
     * vector of keys that don't match.
     *
     * @param tileset_name The name of the tileset to check
     * @param artifact_keys The keys of artifacts to check
     * @return Vector of artifact keys that have mismatched checksums
     */
    [[nodiscard]] virtual std::vector<std::string>
    find_unsynced_artifacts(const std::string &tileset_name, const std::vector<std::string> &artifact_keys) const {
        const auto checksums = compute_artifact_checksums(tileset_name);
        const auto cached_checksums = load_cached_checksums(tileset_name);

        return check_artifact_sync_impl(artifact_keys, checksums, cached_checksums);
    }

    /**
     * @brief Checks if all artifact checksums match their cached values.
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
    [[nodiscard]] virtual bool all_checksums_match(const std::string &tileset_name,
                                                   const std::vector<std::string> &artifact_keys) const {
        return find_unsynced_artifacts(tileset_name, artifact_keys).empty();
    }

  private:
    /**
     * @brief Implementation helper for checking artifact synchronization.
     *
     * @details
     * This method encapsulates the common logic for comparing checksums between current and cached values and returns
     * all mismatched keys.
     *
     * @param artifact_keys The keys to check
     * @param checksums Current checksums
     * @param cached_checksums Cached checksums
     * @return Vector of keys that have mismatched checksums
     */
    [[nodiscard]] static std::vector<std::string>
    check_artifact_sync_impl(const std::vector<std::string> &artifact_keys,
                             const std::unordered_map<std::string, std::string> &checksums,
                             const std::unordered_map<std::string, std::string> &cached_checksums) {
        std::vector<std::string> mismatched_keys;
        for (const auto &key : artifact_keys) {
            const auto checksum_for_key = checksums.contains(key) ? checksums.at(key) : "";
            const auto cached_checksum_for_key = cached_checksums.contains(key) ? cached_checksums.at(key) : "";
            // TODO: more specific error message if one of the above is actually empty?
            if (checksum_for_key != cached_checksum_for_key) {
                mismatched_keys.push_back(key);
            }
        }
        return mismatched_keys;
    }
};

} // namespace porytiles2

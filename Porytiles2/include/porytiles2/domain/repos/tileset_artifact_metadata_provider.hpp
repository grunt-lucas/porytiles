#pragma once

#include <chrono>
#include <filesystem>
#include <ranges>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/templates/result.hpp"

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
 * Among other things, this service is essential for the compilation pipeline to determine when assets need to be
 * recompiled based on changes to source files or existing artifacts.
 */
class TilesetArtifactMetadataProvider {
  public:
    virtual ~TilesetArtifactMetadataProvider() = default;

    /**
     * @brief Gets the keys for all Porytiles artifacts present in the given Tileset.
     *
     * @details
     * Each Porytiles artifact has a unique key by which the ArtifactMetadataProvider and the TilesetRepo can identify
     * it. The format of these keys and the method for producing them are implementation-defined.
     *
     * @return A vector of Porytiles artifact keys for the given Tileset
     */
    [[nodiscard]] virtual std::vector<std::string>
    get_porytiles_artifact_keys(const std::string &tileset_name) const = 0;

    /**
     * @brief Gets the keys for all Porymap artifacts present in the given Tileset.
     *
     * @details
     * Each Porymap artifact has a unique key by which the ArtifactMetadataProvider and the TilesetRepo can identify it.
     * The format of these keys and the method for producing them are implementation-defined.
     *
     * @return A vector of Porymap artifact keys for the given Tileset
     */
    [[nodiscard]] virtual std::vector<std::string> get_porymap_artifact_keys(const std::string &tileset_name) const = 0;

    /**
     * @brief Gets the keys for all artifacts (both Porytiles and Porymap) present in the given Tileset.
     *
     * @details
     * This method combines the results from both get_porytiles_artifact_keys() and get_porymap_artifact_keys() to
     * provide a comprehensive list of all artifact keys associated with the tileset.
     *
     * @param tileset_name The name of the Tileset for which to get all artifact keys
     * @return A vector containing all Porytiles and Porymap artifact keys for the given Tileset
     */
    [[nodiscard]] virtual std::vector<std::string> get_all_artifact_keys(const std::string &tileset_name) const {
        const auto porytiles_keys = get_porytiles_artifact_keys(tileset_name);
        const auto porymap_keys = get_porymap_artifact_keys(tileset_name);

        std::vector<std::string> result;
        result.reserve(porytiles_keys.size() + porymap_keys.size());
        result.insert(result.end(), porytiles_keys.begin(), porytiles_keys.end());
        result.insert(result.end(), porymap_keys.begin(), porymap_keys.end());

        return result;
    }

    /**
     * @brief Computes checksums for the artifacts that belong to the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to compute checksums
     * @return A mapping of artifact keys to their computed checksum
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    compute_artifact_checksums(const std::string &tileset_name) const = 0;

    /**
     * @brief Loads the cached checksums for the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to load cached checksums
     * @return A mapping of artifact keys to their cached checksums
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    load_cached_checksums(const std::string &tileset_name) const = 0;

    /**
     * @brief Caches checksums for the given Tileset to persistent storage.
     *
     * @param tileset_name The name of the Tileset for which to cache checksums
     * @param checksums A mapping of artifact keys to their checksums to be cached
     * @return Result indicating success or failure of the cache operation
     */
    [[nodiscard]] virtual Result<void>
    cache_checksums(const std::string &tileset_name,
                    const std::unordered_map<std::string, std::string> &checksums) const = 0;

    /**
     * @brief Gets the modification timestamps for all Porymap artifacts associated with the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to get timestamps
     * @return A mapping of artifact keys to their modification timestamps
     */
    [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
    get_porymap_timestamps(const std::string &tileset_name) const = 0;

    /**
     * @brief Gets the modification timestamps for all Porytiles artifacts associated with the given Tileset.
     *
     * @param tileset_name The name of the Tileset for which to get timestamps
     * @return A mapping of artifact keys to their modification timestamps
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
    find_unsynced_artifacts(const std::string &tileset_name, const std::vector<std::string> &artifact_keys) const;

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
                                                   const std::vector<std::string> &artifact_keys) const;
};

} // namespace porytiles2

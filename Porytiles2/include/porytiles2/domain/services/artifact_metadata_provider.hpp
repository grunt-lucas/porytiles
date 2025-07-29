#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>

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
     * @brief Computes checksums for the artifacts that correspond to the given Tileset's PorymapTilesetComponent.
     *
     * @param tileset The Tileset for which to compute checksums
     * @return A mapping of artifact identifiers to their computed checksum
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    compute_porymap_checksums(const Tileset &tileset) const = 0;

    /**
     * @brief Loads previously stored checksums for the given tileset.
     *
     * @param tileset_name The name of the tileset for which to load checksums
     * @return A mapping of artifact identifiers to their stored checksums
     */
    [[nodiscard]] virtual std::unordered_map<std::string, std::string>
    load_stored_checksums(const std::string &tileset_name) const = 0;

    /**
     * @brief Stores checksums for the given tileset to persistent storage.
     *
     * @param tileset_name The name of the tileset for which to store checksums
     * @param checksums A mapping of artifact identifiers to their checksums to be stored
     * @return Result indicating success or failure of the storage operation
     */
    [[nodiscard]] virtual Result<void>
    store_checksums(const std::string &tileset_name,
                    const std::unordered_map<std::string, std::string> &checksums) const = 0;

    /**
     * @brief Gets the modification timestamps for all Porymap artifacts associated with the given Tileset.
     *
     * @param tileset_name The name of the tileset for which to get timestamps
     * @return A mapping of artifact identifiers to their modification timestamps
     */
    [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
    get_porymap_timestamps(const std::string &tileset_name) const = 0;

    /**
     * @brief Gets the modification timestamps for all Porytiles artifacts associated with the given Tileset.
     *
     * @param tileset_name The name of the tileset for which to get timestamps
     * @return A mapping of artifact identifiers to their modification timestamps
     */
    [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
    get_porytiles_timestamps(const std::string &tileset_name) const = 0;

    /**
     * @brief Determines if the oldest Porytiles asset is newer than the newest Porymap asset.
     *
     * @param tileset_name The name of the tileset for which to check asset modification times
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
     * @param tileset_name The name of the tileset for which to check asset modification times
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
};

} // namespace porytiles2

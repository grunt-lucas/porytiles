#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

using Timestamp = std::filesystem::file_time_type;

/**
 * @brief Abstract service for managing artifact metadata including checksums and timestamps.
 *
 * @details
 * The ArtifactMetadataProvider provides an interface for accessing and managing metadata
 * about both Porymap and Porytiles artifacts. This includes computing and storing checksums
 * for integrity verification, retrieving modification timestamps, and determining temporal
 * relationships between different artifact sets.
 *
 * This service is essential for the compilation pipeline to determine when assets need
 * to be recompiled based on changes to source files or existing artifacts.
 */
class ArtifactMetadataProvider {
public:
  virtual ~ArtifactMetadataProvider() = default;

  /**
   * @brief Computes checksums for the artifacts that correspond to the given Tileset's
   * PorymapTilesetComponent.
   *
   * @param tileset The Tileset for which to compute checksums.
   * @return A mapping of artifact identifiers to their computed checksum.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  ComputePorymapChecksums(const Tileset &tileset) const = 0;

  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  LoadStoredChecksums(const std::string &tileset_name) const = 0;

  [[nodiscard]] virtual Result<void>
  StoreChecksums(const std::string &tileset_name,
                 const std::unordered_map<std::string, std::string> &checksums) = 0;

  /**
   * @brief Gets the modification timestamps for all Porymap artifacts associated with the given
   * Tileset.
   *
   * @param tileset The Tileset for which to get Porymap artifact timestamps.
   * @return A mapping of artifact identifiers to their modification timestamps.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
  GetPorymapTimestamps(const Tileset &tileset) const = 0;

  /**
   * @brief Gets the modification timestamps for all Porytiles artifacts associated with the given
   * Tileset.
   *
   * @param tileset The Tileset for which to get Porytiles artifact timestamps.
   * @return A mapping of artifact identifiers to their modification timestamps.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, Timestamp>
  GetPorytilesTimestamps(const Tileset &tileset) const = 0;

  /**
   * @brief Determines if any Porymap assets are newer than any Porytiles assets.
   *
   * @param tileset The Tileset to check.
   * @return True if Porymap assets are newer, false otherwise.
   */
  [[nodiscard]] virtual bool ArePorymapAssetsNewer(const Tileset &tileset) const = 0;
};

} // namespace porytiles

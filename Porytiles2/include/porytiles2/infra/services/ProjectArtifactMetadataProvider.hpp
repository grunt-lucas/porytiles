// Porytiles2/include/porytiles2/infra/services/FilesystemTimestampService.hpp
#pragma once

#include "porytiles2/domain/services/ArtifactMetadataProvider.hpp"

namespace porytiles {

/**
 * @brief An implementation of the ArtifactMetadataProvider service that uses an in-filesystem
 * `pokeemerald` project as the source for artifact metadata.
 */
class ProjectArtifactMetadataProvider final : public ArtifactMetadataProvider {
public:
  ProjectArtifactMetadataProvider() = default;

  [[nodiscard]] std::unordered_map<std::string, std::string>
  compute_porymap_checksums(const Tileset &tileset) const override;

  [[nodiscard]] std::unordered_map<std::string, std::string>
  load_stored_checksums(const std::string &tileset_name) const override;

  [[nodiscard]] Result<void>
  store_checksums(const std::string &tileset_name,
                  const std::unordered_map<std::string, std::string> &checksums) override;

  [[nodiscard]] std::unordered_map<std::string, Timestamp>
  get_porymap_timestamps(const Tileset &tileset) const override;

  [[nodiscard]] std::unordered_map<std::string, Timestamp>
  get_porytiles_timestamps(const Tileset &tileset) const override;

  [[nodiscard]] bool are_porymap_assets_newer(const Tileset &tileset) const override;
};

} // namespace porytiles

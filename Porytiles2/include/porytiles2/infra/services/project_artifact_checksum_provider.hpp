#pragma once

#include <filesystem>

#include "porytiles2/domain/services/artifact_checksum_provider.hpp"

namespace porytiles2 {

/**
 * @brief TODO: fill in doxygen
 *
 * @details TODO: fill in doxygen
 */
class ProjectArtifactChecksumProvider final : public ArtifactChecksumProvider {
  public:
    explicit ProjectArtifactChecksumProvider(std::filesystem::path project_root)
        : project_root_{std::move(project_root)}
    {
    }

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    compute_artifact_checksums(const std::string &tileset_name) const override;

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    load_cached_checksums(const std::string &tileset_name) const override;

    [[nodiscard]] Result<void> cache_checksums(
        const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const override;

  private:
    std::filesystem::path project_root_;
};

} // namespace porytiles2

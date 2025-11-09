#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/services/artifact_checksum_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief TODO: fill in doxygen
 *
 * @details TODO: fill in doxygen
 */
class ProjectArtifactChecksumProvider final : public ArtifactChecksumProvider {
  public:
    explicit ProjectArtifactChecksumProvider(gsl::not_null<ProjectTilesetArtifactKeyProvider *> key_provider)
        : key_provider_{key_provider}
    {
    }

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    compute_tileset_artifact_checksums(const std::string &tileset_name) const override;

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    load_cached_tileset_checksums(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<void> cache_tileset_checksums(
        const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const override;

  private:
    ProjectTilesetArtifactKeyProvider *key_provider_;
};

} // namespace porytiles2

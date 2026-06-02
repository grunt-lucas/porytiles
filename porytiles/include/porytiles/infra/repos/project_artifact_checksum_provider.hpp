#pragma once

#include <filesystem>
#include <string>

#include "porytiles/domain/repos/artifact_checksum_provider.hpp"
#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/**
 * @brief Pokeemerald project filesystem-based implementation of ArtifactChecksumProvider.
 *
 * @details
 * This class computes and caches checksums for tileset artifacts stored in a pokeemerald project. Checksums are
 * persisted as JSON in the porytiles utility directory at `porytiles/tilesets/{tileset_name}/tileset.cache.json`.
 *
 * Key paths are relativized against the project root before being stored, ensuring checksums remain valid even when
 * the project directory is moved.
 *
 * @see ArtifactChecksumProvider for the abstract interface
 */
class ProjectArtifactChecksumProvider final : public ArtifactChecksumProvider {
  public:
    explicit ProjectArtifactChecksumProvider(std::filesystem::path project_root)
        : project_root_{std::move(project_root)}
    {
    }

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    compute_tileset_artifact_checksums(const std::vector<ArtifactKey> &keys) const override;

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    load_cached_tileset_checksums(const std::string &tileset_name) const override;

    [[nodiscard]] ChainableResult<void> cache_tileset_checksums(
        const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const override;

  private:
    const std::filesystem::path project_root_;
};

} // namespace porytiles

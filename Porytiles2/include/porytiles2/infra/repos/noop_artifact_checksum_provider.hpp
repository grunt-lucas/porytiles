#pragma once

#include <string>

#include "porytiles2/domain/repos/artifact_checksum_provider.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of ArtifactChecksumProvider that just does nothing.
 *
 * @details
 * This implementation provides no-op behavior for all checksum operations, returning empty results
 * and always indicating success. It's useful for testing or when checksum functionality is not needed.
 */
class NoopArtifactChecksumProvider final : public ArtifactChecksumProvider {
  public:
    NoopArtifactChecksumProvider() = default;

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    compute_tileset_artifact_checksums(const std::string &name) const override;

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    load_cached_tileset_checksums(const std::string &name) const override;

    [[nodiscard]] ChainableResult<void> cache_tileset_checksums(
        const std::string &name, const std::unordered_map<ArtifactKey, std::string> &checksums) const override;
};

} // namespace porytiles2

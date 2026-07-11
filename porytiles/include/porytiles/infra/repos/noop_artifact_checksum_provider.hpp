#pragma once

#include <string>
#include <vector>

#include "porytiles/domain/repos/artifact_checksum_provider.hpp"
#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief An implementation of ArtifactChecksumProvider that just does nothing.
///
/// @details
/// This implementation provides no-op behavior for all checksum operations, returning empty results
/// and always indicating success. It's useful for testing or when checksum functionality is not needed.
class NoopArtifactChecksumProvider final : public ArtifactChecksumProvider {
  public:
    NoopArtifactChecksumProvider() = default;

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    compute_tileset_artifact_checksums(const std::vector<ArtifactKey> &keys) const override;

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    load_cached_tileset_checksums(const std::string &name) const override;

    [[nodiscard]] ChainableResult<void> cache_tileset_checksums(
        const std::string &name, const std::unordered_map<ArtifactKey, std::string> &checksums) const override;
};

} // namespace porytiles

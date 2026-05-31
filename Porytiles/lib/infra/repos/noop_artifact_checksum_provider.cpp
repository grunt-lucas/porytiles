#include "porytiles/infra/repos/noop_artifact_checksum_provider.hpp"

#include <string>
#include <unordered_map>

#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

std::unordered_map<ArtifactKey, std::string>
NoopArtifactChecksumProvider::compute_tileset_artifact_checksums(const std::vector<ArtifactKey> & /*keys*/) const
{
    return {};
}

std::unordered_map<ArtifactKey, std::string>
NoopArtifactChecksumProvider::load_cached_tileset_checksums(const std::string & /*name*/) const
{
    return {};
}

ChainableResult<void> NoopArtifactChecksumProvider::cache_tileset_checksums(
    const std::string & /*name*/, const std::unordered_map<ArtifactKey, std::string> & /*checksums*/) const
{
    return {};
}

} // namespace porytiles

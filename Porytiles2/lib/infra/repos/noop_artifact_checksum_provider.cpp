#include "porytiles2/infra/repos/noop_artifact_checksum_provider.hpp"

#include <string>
#include <unordered_map>

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

std::unordered_map<ArtifactKey, std::string>
NoopArtifactChecksumProvider::compute_tileset_artifact_checksums(const std::string & /*name*/) const
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

} // namespace porytiles2

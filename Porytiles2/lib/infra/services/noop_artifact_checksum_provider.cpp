#include "porytiles2/infra/services/noop_artifact_checksum_provider.hpp"

#include <string>
#include <unordered_map>

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

std::unordered_map<ArtifactKey, std::string>
NoopArtifactChecksumProvider::compute_tileset_artifact_checksums(const std::string &tileset_name) const
{
    return {};
}

std::unordered_map<ArtifactKey, std::string>
NoopArtifactChecksumProvider::load_cached_tileset_checksums(const std::string &tileset_name) const
{
    return {};
}

Result<void> NoopArtifactChecksumProvider::cache_tileset_checksums(
    const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const
{
    return {};
}

} // namespace porytiles2

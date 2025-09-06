#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"

namespace porytiles2 {

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::compute_artifact_checksums(const std::string &tileset_name) const
{
}

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::load_cached_checksums(const std::string &tileset_name) const
{
}

Result<void> ProjectArtifactChecksumProvider::cache_checksums(
    const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const
{
}

} // namespace porytiles2

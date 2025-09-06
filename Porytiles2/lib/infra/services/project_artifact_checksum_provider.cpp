#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"

#include <iostream>

namespace porytiles2 {

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::compute_artifact_checksums(const std::string &tileset_name) const
{
    // TODO: implement
    return {};
}

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::load_cached_checksums(const std::string &tileset_name) const
{
    // TODO: implement
    const auto keys = key_provider_->get_all_artifact_keys(tileset_name);
    for (const auto &key : keys) {
        std::cerr << key.key() << std::endl;
    }
    return {};
}

Result<void> ProjectArtifactChecksumProvider::cache_checksums(
    const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const
{
    // TODO: implement
    return {};
}

} // namespace porytiles2

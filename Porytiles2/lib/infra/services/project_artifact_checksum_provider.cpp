#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"

#include <filesystem>
#include <fstream>

#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::compute_tileset_artifact_checksums(const std::string &tileset_name) const
{
    const auto &all_keys = key_provider_->get_all_artifact_keys(tileset_name);
    return {};
}

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::load_cached_tileset_checksums(const std::string &tileset_name) const
{
    // TODO: tileset checksum file location should be configurable?
    const auto artifact_checksum_file = key_provider_->tileset_root(tileset_name) / "artifact_checksums.json";

    if (!exists(artifact_checksum_file)) {
        panic(fmt::format("expected checksum file '{}' does not exist", artifact_checksum_file.string()));
    }

    std::ifstream file{artifact_checksum_file};
    nlohmann::json json_data;
    file >> json_data;

    std::unordered_map<ArtifactKey, std::string> checksums;
    for (const auto &[key, value] : json_data.items()) {
        const auto full_path = key_provider_->tileset_root(tileset_name) / std::filesystem::path{key};
        checksums.emplace(ArtifactKey{full_path}, value.get<std::string>());
    }

    return checksums;
}

Result<void> ProjectArtifactChecksumProvider::cache_tileset_checksums(
    const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const
{
    // TODO: implement
    // TODO: don't save the full path, subtract the tileset_root from the saved key
    return {};
}

} // namespace porytiles2

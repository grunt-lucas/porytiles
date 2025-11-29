#include "porytiles2/domain/services/artifact_checksum_provider.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace porytiles2 {

std::vector<ArtifactKey> ArtifactChecksumProvider::find_unsynced_tileset_artifacts(
    const std::string &tileset_name, const std::vector<ArtifactKey> &artifact_keys) const
{
    const auto checksums = compute_tileset_artifact_checksums(tileset_name);
    const auto cached_checksums = load_cached_tileset_checksums(tileset_name);

    if (cached_checksums.empty()) {
        return {};
    }

    std::vector<ArtifactKey> mismatched_keys;
    for (const auto &key : artifact_keys) {
        const auto checksum_for_key = checksums.contains(key) ? checksums.at(key) : "";
        const auto cached_checksum_for_key = cached_checksums.contains(key) ? cached_checksums.at(key) : "";
        if (checksum_for_key != cached_checksum_for_key) {
            mismatched_keys.push_back(key);
        }
    }
    return mismatched_keys;
}

bool ArtifactChecksumProvider::cached_checksums_exist(const std::string &tileset_name) const
{
    return !load_cached_tileset_checksums(tileset_name).empty();
}

bool ArtifactChecksumProvider::all_checksums_tileset_match(
    const std::string &tileset_name, const std::vector<ArtifactKey> &artifact_keys) const
{
    return find_unsynced_tileset_artifacts(tileset_name, artifact_keys).empty();
}

} // namespace porytiles2

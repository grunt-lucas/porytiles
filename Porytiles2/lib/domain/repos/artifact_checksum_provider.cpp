#include "porytiles2/domain/repos/artifact_checksum_provider.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace porytiles2 {

std::vector<ArtifactKey> ArtifactChecksumProvider::find_unsynced_artifacts(
    const std::string &tileset_name, const std::vector<ArtifactKey> &artifact_keys) const {
    const auto checksums = compute_artifact_checksums(tileset_name);
    const auto cached_checksums = load_cached_checksums(tileset_name);

    std::vector<ArtifactKey> mismatched_keys;
    for (const auto &key : artifact_keys) {
        const auto checksum_for_key = checksums.contains(key) ? checksums.at(key) : "";
        const auto cached_checksum_for_key = cached_checksums.contains(key) ? cached_checksums.at(key) : "";
        // TODO: more specific error message if one of the above is actually empty?
        if (checksum_for_key != cached_checksum_for_key) {
            mismatched_keys.push_back(key);
        }
    }
    return mismatched_keys;
}

bool ArtifactChecksumProvider::all_checksums_match(
    const std::string &tileset_name, const std::vector<ArtifactKey> &artifact_keys) const {
    return find_unsynced_artifacts(tileset_name, artifact_keys).empty();
}

} // namespace porytiles2

#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"

#include <filesystem>
#include <fstream>
#include <map>

#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/templates/panic.hpp"
#include "porytiles2/templates/stream_digest.hpp"

namespace porytiles2 {

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::compute_tileset_artifact_checksums(const std::string &tileset_name) const
{
    const StreamDigest digest{};
    std::unordered_map<ArtifactKey, std::string> checksums{};
    const auto &all_keys = key_provider_->get_all_artifact_keys(tileset_name);

    for (const auto &key : all_keys) {
        if (!key_provider_->artifact_exists(key)) {
            panic(fmt::format("expected artifact '{}' does not exist", key.key()));
        }
        std::ifstream stream{key.key()};
        const auto key_digest = digest.digest(stream);
        checksums.emplace(key, key_digest);
    }

    return checksums;
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
    // TODO: tileset checksum file location should be configurable?
    const auto artifact_checksum_file = key_provider_->tileset_root(tileset_name) / "artifact_checksums.json";
    std::ofstream file{artifact_checksum_file};

    // First, collect all relative paths with their checksums
    std::map<std::string, std::string> sorted_checksums;
    for (const auto &[artifact_key, checksum] : checksums) {
        const auto relative_path =
            std::filesystem::relative(artifact_key.key(), key_provider_->tileset_root(tileset_name));
        sorted_checksums[relative_path.string()] = checksum;
    }

    /*
     * Technically, the above sorting step is unnecessary, since by default the nlohmann json library automatically
     * sorts json keys alphabetically (the underlying implementation uses std::map). However, we explicitly sort them
     * here in case this implementation ever changes, or we change json libraries.
     *
     * https://json.nlohmann.me/features/object_order/#default-behavior-sort-keys
     */

    // Now build JSON from the sorted map, ensuring consistent ordering
    nlohmann::json json_data;
    for (const auto &[path, checksum] : sorted_checksums) {
        json_data[path] = checksum;
    }

    // prettify the json with 2-space indentation
    file << json_data.dump(2);
    return {};
}

} // namespace porytiles2

#include "porytiles/infra/repos/project_artifact_checksum_provider.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <map>

#include "nlohmann/json.hpp"

#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/stream_digest.hpp"

namespace {

std::filesystem::path checksums_file(const std::filesystem::path &project_root, const std::string &tileset_name)
{
    return project_root / "porytiles" / "tilesets" / tileset_name / "tileset.cache.json";
}

} // namespace

namespace porytiles {

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::compute_tileset_artifact_checksums(const std::vector<ArtifactKey> &keys) const
{
    std::unordered_map<ArtifactKey, std::string> checksums{};

    for (const auto &key : keys) {
        constexpr StreamDigest digest{};
        // Keys are relative to project_root_, so prepend for filesystem operations
        const auto absolute_path = project_root_ / key.key();
        if (!std::filesystem::exists(absolute_path)) {
            panic(std::format("expected artifact '{}' does not exist", absolute_path.string()));
        }
        std::ifstream stream{absolute_path};
        const auto key_digest = digest.digest(stream);
        checksums.emplace(key, key_digest);
    }

    return checksums;
}

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::load_cached_tileset_checksums(const std::string &tileset_name) const
{
    const auto artifact_checksum_path = checksums_file(project_root_, tileset_name);

    // If checksum file doesn't exist, just return nothing
    if (!exists(artifact_checksum_path)) {
        return {};
    }

    std::ifstream file{artifact_checksum_path};
    nlohmann::json json_data;
    file >> json_data;

    std::unordered_map<ArtifactKey, std::string> checksums;
    for (const auto &[key, value] : json_data.items()) {
        const auto full_path = std::filesystem::path{key};
        checksums.emplace(ArtifactKey{full_path}, value.get<std::string>());
    }

    return checksums;
}

ChainableResult<void> ProjectArtifactChecksumProvider::cache_tileset_checksums(
    const std::string &tileset_name, const std::unordered_map<ArtifactKey, std::string> &checksums) const
{
    const auto artifact_checksum_file = checksums_file(project_root_, tileset_name);
    std::filesystem::create_directories(artifact_checksum_file.parent_path());
    std::ofstream file{artifact_checksum_file};

    // Collect all keys with their checksums in sorted order for consistent JSON output
    std::map<std::string, std::string> sorted_checksums;
    for (const auto &[artifact_key, checksum] : checksums) {
        // Keys are already relative to project_root_, so use them directly
        sorted_checksums[artifact_key.key()] = checksum;
    }

    // Technically, the above sorting step is unnecessary, since by default the nlohmann json library automatically
    // sorts json keys alphabetically (the underlying implementation uses std::map). However, we explicitly sort them
    // here in case this implementation ever changes, or we change json libraries.
    //
    // https://json.nlohmann.me/features/object_order/#default-behavior-sort-keys

    // Now build JSON from the sorted map, ensuring consistent ordering
    nlohmann::json json_data;
    for (const auto &[path, checksum] : sorted_checksums) {
        json_data[path] = checksum;
    }

    // prettify the json with 2-space indentation
    file << json_data.dump(2);
    file << std::endl;
    return {};
}

} // namespace porytiles

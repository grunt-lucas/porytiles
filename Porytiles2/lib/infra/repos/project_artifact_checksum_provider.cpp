#include "porytiles2/infra/repos/project_artifact_checksum_provider.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <map>

#include "nlohmann/json.hpp"

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/stream_digest.hpp"

namespace porytiles2 {

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::compute_tileset_artifact_checksums(const std::string &name) const
{
    std::unordered_map<ArtifactKey, std::string> checksums{};

    auto all_keys_result = key_provider_->get_all_artifact_keys(name);
    if (!all_keys_result.has_value()) {
        // This is called after a successful save, so artifact keys should be valid
        panic(std::format("failed to get artifact keys for tileset '{}'", name));
    }
    for (const auto &key : all_keys_result.value()) {
        constexpr StreamDigest digest{};
        if (!key_provider_->artifact_exists(key)) {
            panic(std::format("expected artifact '{}' does not exist", key.key()));
        }
        std::ifstream stream{key.key()};
        const auto key_digest = digest.digest(stream);
        checksums.emplace(key, key_digest);
    }

    return checksums;
}

std::unordered_map<ArtifactKey, std::string>
ProjectArtifactChecksumProvider::load_cached_tileset_checksums(const std::string &name) const
{
    // TODO: tileset checksum file location should be configurable?
    auto tileset_root_result = key_provider_->tileset_root(name);
    if (!tileset_root_result.has_value()) {
        // If we can't get tileset root, return empty checksums
        return {};
    }
    const auto &tileset_root = tileset_root_result.value();
    const auto artifact_checksum_file = tileset_root / "artifact_checksums.json";

    // If checksum file doesn't exist, just return nothing
    if (!exists(artifact_checksum_file)) {
        return {};
    }

    std::ifstream file{artifact_checksum_file};
    nlohmann::json json_data;
    file >> json_data;

    std::unordered_map<ArtifactKey, std::string> checksums;
    for (const auto &[key, value] : json_data.items()) {
        const auto full_path = tileset_root / std::filesystem::path{key};
        checksums.emplace(ArtifactKey{full_path}, value.get<std::string>());
    }

    return checksums;
}

ChainableResult<void> ProjectArtifactChecksumProvider::cache_tileset_checksums(
    const std::string &name, const std::unordered_map<ArtifactKey, std::string> &checksums) const
{
    // TODO: tileset checksum file location should be configurable?
    PT_TRY_ASSIGN_CHAIN_ERR(tileset_root, key_provider_->tileset_root(name), "failed to cache tileset checksums", void);
    const auto artifact_checksum_file = tileset_root / "artifact_checksums.json";
    std::ofstream file{artifact_checksum_file};

    // First, collect all relative paths with their checksums
    std::map<std::string, std::string> sorted_checksums;
    for (const auto &[artifact_key, checksum] : checksums) {
        /*
         * Before saving the checksums, relativize the key's path against the tileset root. Since the user might call
         * the command from different working directories, it shouldn't save the path relative to the working directory.
         * It also doesn't really make sense to save an absolute path here, since users might rename the tileset or move
         * their project folder.
         */
        const auto relative_key = std::filesystem::relative(artifact_key.key(), tileset_root);
        sorted_checksums[relative_key] = checksum;
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

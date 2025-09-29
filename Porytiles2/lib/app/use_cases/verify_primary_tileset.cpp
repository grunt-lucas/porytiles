#include "porytiles2/app/use_cases/verify_primary_tileset.hpp"

#include <string>

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

[[nodiscard]] ChainableResult<void> VerifyPrimaryTileset::verify(const std::string &tileset_name) const
{
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(tileset_name)) {
        return ChainableResult<void>{
            FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}}};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        // TODO: hook up ChainableError here
        return ChainableResult<void>::chain_together(
            FormattableError{"failed to load tileset '{}'", FormatParam{tileset_name, Style::bold}}, maybe_tileset);
    }
    const auto tileset = std::move(maybe_tileset.value());

    // TODO: should we split this into separate handling of Porymap and Porytiles assets?
    const auto artifact_keys = tileset_repo_->key_provider().get_all_artifact_keys(tileset_name);
    const auto mismatched_keys =
        tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(tileset_name, artifact_keys);
    if (!mismatched_keys.empty()) {
        std::vector<FormatParam> keys;
        keys.reserve(mismatched_keys.size());
        for (const auto &key : mismatched_keys) {
            keys.push_back(FormatParam{key.key(), Style::bold});
        }
        // TODO: create some kind of MultilineFormattableError that can correctly format a multiline message
        return ChainableResult<void>{FormattableError{"changes present in tileset assets: {}", keys}};
    }

    return {};
}

} // namespace porytiles2

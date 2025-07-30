#include "porytiles2/app/use_cases/import_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<void> ImportPrimaryTileset::import(const std::string &tileset_name) const {
    // 1. Check if the primary tileset exists. If not, abort with error.
    if (!tileset_repo_->exists(tileset_name)) {
        return std::unexpected{fmt::format("tileset {} does not exist", tileset_name)};
    }

    // 2. Load the tileset into a `Tileset` aggregate.
    auto maybe_tileset = tileset_repo_->load(tileset_name);
    if (!maybe_tileset.has_value()) {
        return std::unexpected{maybe_tileset.error()};
    }
    const auto tileset = std::move(maybe_tileset.value());

    return {};
}

} // namespace porytiles2

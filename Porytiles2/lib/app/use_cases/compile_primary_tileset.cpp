#include <porytiles2/app/use_cases/compile_primary_tileset.hpp>

#include <expected>
#include <memory>

#include <fmt/format.h>

#include <porytiles2/domain/services/tileset_compiler_service.hpp>
#include <porytiles2/templates/result.hpp>

namespace porytiles {

Result<void> CompilePrimaryTileset::Compile(const std::string &tileset) const {
    // 1. Load the PorytilesTileset source aggregate
    auto maybe_porytiles_tileset = porytiles_repo_->Load(tileset);
    if (!maybe_porytiles_tileset.has_value()) {
        return std::unexpected{fmt::format("failed to load tileset '{}'", tileset)};
    }
    const std::unique_ptr<PorytilesTileset> porytiles_tileset = std::move(maybe_porytiles_tileset.value());

    // 2. Compile with the TilesetCompilerService domain service
    auto maybe_porymap_tileset = compiler_service_->CompilePrimary(*porytiles_tileset);
    if (!maybe_porymap_tileset.has_value()) {
        return std::unexpected{fmt::format("failed to compile tileset '{}'", tileset)};
    }

    // 3. Save the resulting PorymapTileset aggregate
    const std::unique_ptr<PorymapTileset> porymap_tileset = std::move(maybe_porymap_tileset.value());
    if (const auto maybe_save_result = porymap_repo_->Save(*porymap_tileset); !maybe_save_result.has_value()) {
        return std::unexpected{fmt::format("failed to save tileset '{}'", tileset)};
    }
    return {};
}

} // namespace porytiles

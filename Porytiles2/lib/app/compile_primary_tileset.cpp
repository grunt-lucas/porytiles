#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/infra/diagnostics/diagnostic_engine_factory.hpp"

#include <porytiles2/app/compile_primary_tileset.hpp>
#include <porytiles2/domain/services/tileset_compiler_service.hpp>

namespace porytiles {

void CompilePrimaryTileset::Compile(const std::string &tileset) const {
    const auto diag = &DiagEngineFactory::GetEngine();

    // 1. Load the PorytilesTileset source aggregate
    auto maybe_porytiles_tileset = porytiles_repo_->load(tileset);
    if (!maybe_porytiles_tileset.has_value()) {
        diag->Report(FatalGeneric, maybe_porytiles_tileset.error());
        return;
    }
    const std::unique_ptr<PorytilesTileset> porytiles_tileset = std::move(maybe_porytiles_tileset.value());

    // 2. Compile with the TilesetCompilerService domain service
    auto maybe_porymap_tileset = compiler_service_->CompilePrimary(*porytiles_tileset);
    if (!maybe_porymap_tileset.has_value()) {
        diag->Report(FatalGeneric, maybe_porytiles_tileset.error());
        return;
    }

    // 3. Save the resulting PorymapTileset aggregate
    const std::unique_ptr<PorymapTileset> porymap_tileset = std::move(maybe_porymap_tileset.value());
    porymap_repo_->save(*porymap_tileset);
}

} // namespace porytiles

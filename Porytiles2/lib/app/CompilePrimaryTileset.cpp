#include "porytiles2/app/CompilePrimaryTileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/TilesetCompilerService.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void> CompilePrimaryTileset::Compile(const std::string &tileset) const {
  // TODO : these steps should match those laid out in ARCHITECTURE.md
  // // 1. Load the PorytilesTileset source aggregate
  // auto maybe_porytiles_tileset = porytiles_repo_->Load(tileset);
  // if (!maybe_porytiles_tileset.has_value()) {
  //   return std::unexpected{maybe_porytiles_tileset.error()};
  // }
  // const auto porytiles_tileset = std::move(maybe_porytiles_tileset.value());

  // // 2. Compile with the TilesetCompilerService domain service
  // auto maybe_porymap_tileset =
  //     compiler_service_->CompilePrimary(*porytiles_tileset);
  // if (!maybe_porymap_tileset.has_value()) {
  //   return std::unexpected{maybe_porymap_tileset.error()};
  // }

  // // 3. Save the resulting PorymapTileset aggregate
  // const auto porymap_tileset = std::move(maybe_porymap_tileset.value());
  // if (const auto maybe_save_result = porymap_repo_->Save(*porymap_tileset);
  //     !maybe_save_result.has_value()) {
  //   return std::unexpected{maybe_save_result.error()};
  // }
  return {};
}

} // namespace porytiles

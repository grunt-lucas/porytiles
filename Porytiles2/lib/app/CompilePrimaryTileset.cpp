#include "porytiles2/app/CompilePrimaryTileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/TilesetCompiler.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void> CompilePrimaryTileset::Compile(const std::string &tileset_name) const {
  // 1. Load tileset
  auto maybe_tileset = tileset_repo_->Load(tileset_name);
  if (!maybe_tileset.has_value()) {
    return std::unexpected{maybe_tileset.error()};
  }
  const auto tileset = std::move(maybe_tileset.value());

  // 2. Check for unimported changes
  auto current_checksums = checksum_service_->ComputePorymapChecksums(*tileset);
  auto stored_checksums = checksum_service_->LoadStoredChecksums(tileset_name);

  for (const auto &[artifact, current_sum] : current_checksums) {
    if (auto stored_it = stored_checksums.find(artifact);
        stored_it != stored_checksums.end() && stored_it->second != current_sum) {
      return std::unexpected{"unimported changes present in Porymap asset " + artifact};
    }
  }

  // 3. Exit early if no changes in Porytiles assets to compile
  if (timestamp_service_->ArePorymapAssetsNewer(*tileset)) {
    // nothing to do - Porymap assets are newer than Porytiles assets
    // TODO : display this message to the user
    return {};
  }

  // 4. Perform compilation logic
  const auto porytiles_component = tileset->porytiles_component();
  auto maybe_porymap_component = compiler_service_->CompilePrimary(porytiles_component);
  if (!maybe_porymap_component.has_value()) {
    return std::unexpected{maybe_porymap_component.error()};
  }
  auto porymap_component = std::move(maybe_porymap_component.value());

  // 5. Update tileset with new Porymap component
  tileset->porymap_component(std::move(porymap_component));

  // 6. Persist updated tileset with updated Porymap artifact checksums
  if (const auto save_result = tileset_repo_->Save(*tileset); !save_result.has_value()) {
    return std::unexpected{save_result.error()};
  }

  return {};
}

} // namespace porytiles

#pragma once

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorymapTileset.hpp"
#include "porytiles2/domain/repos/PorymapTilesetRepo.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Implementation of PorymapTilesetRepo that uses an in-filesystem
 * `pokeemerald` project as the backing store.
 */
class ProjectPorymapTilesetRepo final : public PorymapTilesetRepo {
public:
  Result<void> Save(const PorymapTileset &tileset) override;

  Result<std::unique_ptr<PorymapTileset>>
  Load(const std::string &name) override;
};

} // namespace porytiles

#pragma once

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/domain/repos/TilesetRepo.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Implementation of TilesetRepo that uses an in-filesystem `pokeemerald`
 * project as the backing store.
 */
class ProjectTilesetRepo final : public TilesetRepo {
public:
  Result<void> Save(const Tileset &tileset) override;

  Result<std::unique_ptr<Tileset>> Load(const std::string &name) override;
};

} // namespace porytiles

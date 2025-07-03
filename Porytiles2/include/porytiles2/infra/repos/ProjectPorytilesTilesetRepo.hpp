#pragma once

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/PorytilesTileset.hpp"
#include "porytiles2/domain/repos/PorytilesTilesetRepo.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Implementation of PorytilesTilesetRepo that uses an in-filesystem
 * `pokeemerald` project as the backing store.
 */
class ProjectPorytilesTilesetRepo final : public PorytilesTilesetRepo {
public:
  Result<void> Save(const PorytilesTileset &tileset) override;

  Result<std::unique_ptr<PorytilesTileset>>
  Load(const std::string &name) override;
};

} // namespace porytiles

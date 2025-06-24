#pragma once

#include <expected>
#include <memory>
#include <string>

#include <porytiles2/domain/aggregates/porytiles_tileset.hpp>
#include <porytiles2/domain/repos/porytiles_tileset_repo.hpp>
#include <porytiles2/templates/result.hpp>

namespace porytiles {

/**
 * @brief Implementation of PorytilesTilesetRepo that uses an in-filesystem `pokeemerald` project as the backing store.
 */
class ProjectPorytilesTilesetRepo final : public PorytilesTilesetRepo {
  public:
    void Save(const PorytilesTileset &tileset) override;

    Result<std::unique_ptr<PorytilesTileset>> Load(const std::string &name) override;
};

} // namespace porytiles

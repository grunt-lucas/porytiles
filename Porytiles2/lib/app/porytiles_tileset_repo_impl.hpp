#pragma once

#include <expected>
#include <memory>
#include <string>

#include <porytiles2/app/porytiles_tileset_repo.hpp>
#include <porytiles2/domain/tilesets/porytiles_tileset.hpp>

namespace porytiles {

class PorytilesTilesetRepoImpl final : public PorytilesTilesetRepo {
  public:
    void save(const PorytilesTileset &tileset) override;

    std::expected<std::unique_ptr<PorytilesTileset>, std::string> load(const std::string &name) override;
};

} // namespace porytiles

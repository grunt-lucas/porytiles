#pragma once

#include <expected>
#include <memory>
#include <string>

#include <porytiles2/domain/aggregates/porytiles_tileset.hpp>
#include <porytiles2/domain/repos/porytiles_tileset_repo.hpp>
#include <porytiles2/templates/result.hpp>

namespace porytiles {

class FilesystemPorytilesTilesetRepo final : public PorytilesTilesetRepo {
  public:
    void save(const PorytilesTileset &tileset) override;

    Result<std::unique_ptr<PorytilesTileset>> load(const std::string &name) override;
};

} // namespace porytiles

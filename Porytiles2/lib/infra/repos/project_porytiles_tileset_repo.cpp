#include "porytiles2/infra/repos/project_porytiles_tileset_repo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/porytiles_tileset.hpp"
#include "porytiles2/templates/panic.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles {

Result<void> ProjectPorytilesTilesetRepo::Save(const PorytilesTileset &tileset) {
    // TODO : impl
    Panic("unimplemented");
}

Result<std::unique_ptr<PorytilesTileset>> ProjectPorytilesTilesetRepo::Load(const std::string &name) {
    // TODO : impl
    Panic("unimplemented");
}

} // namespace porytiles

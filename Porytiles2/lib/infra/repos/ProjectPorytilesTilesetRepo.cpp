#include "porytiles2/infra/repos/ProjectPorytilesTilesetRepo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorytilesTileset.hpp"
#include "porytiles2/templates/Panic.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void>
ProjectPorytilesTilesetRepo::Save(const PorytilesTileset &tileset) {
  // TODO : impl
  Panic("unimplemented");
}

Result<std::unique_ptr<PorytilesTileset>>
ProjectPorytilesTilesetRepo::Load(const std::string &name) {
  // TODO : impl
  Panic("unimplemented");
}

} // namespace porytiles

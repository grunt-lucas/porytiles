#include "porytiles2/infra/repos/project/ProjectTilesetRepo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/templates/Panic.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void> ProjectTilesetRepo::Save(const Tileset &tileset) {
  // TODO : impl
  Panic("unimplemented");
}

Result<std::unique_ptr<Tileset>>
ProjectTilesetRepo::Load(const std::string &name) {
  // TODO : impl
  Panic("unimplemented");
}

} // namespace porytiles

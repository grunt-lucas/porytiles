#include "../../../include/porytiles2/infra/repos/ProjectTilesetRepo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/templates/Panic.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<std::unique_ptr<Tileset>> ProjectTilesetRepo::Load(const std::string &name) const {
  // TODO : impl
  Panic("unimplemented");
}

bool ProjectTilesetRepo::Exists(const std::string &name) const {
  // TODO : impl
  Panic("unimplemented");
}

Result<void> ProjectTilesetRepo::SaveTileset(const Tileset &tileset) {
  // TODO : impl
  Panic("unimplemented");
}

} // namespace porytiles

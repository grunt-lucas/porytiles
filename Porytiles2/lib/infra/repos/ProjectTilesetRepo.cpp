#include "../../../include/porytiles2/infra/repos/ProjectTilesetRepo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/templates/Panic.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles2 {

Result<std::unique_ptr<Tileset>> ProjectTilesetRepo::load(const std::string &name) const {
  // TODO : impl
  panic("unimplemented");
}

bool ProjectTilesetRepo::exists(const std::string &name) const {
  // TODO : impl
  panic("unimplemented");
}

Result<void> ProjectTilesetRepo::save_tileset(const Tileset &tileset) {
  // TODO : impl
  panic("unimplemented");
}

} // namespace porytiles2

#include "porytiles2/infra/repos/project_tileset_repo.hpp"

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/templates/panic.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

std::unique_ptr<Tileset> ProjectTilesetRepo::create_empty_tileset() const {
    // TODO: this should return a ProjectTileset
    return std::make_unique<Tileset>();
}

bool ProjectTilesetRepo::exists(const std::string &name) const {
    // TODO : impl
    panic("unimplemented");
}

} // namespace porytiles2

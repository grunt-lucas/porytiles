#include <app/porytiles_tileset_repo_impl.hpp>

#include <expected>
#include <memory>
#include <string>

#include <porytiles2/domain/tilesets/porytiles_tileset.hpp>
#include <porytiles2/templates/panic.hpp>

namespace porytiles {

void PorytilesTilesetRepoImpl::save(const PorytilesTileset &tileset) {
    // TODO : impl
    Panic("unimplemented");
}

std::expected<std::unique_ptr<PorytilesTileset>, std::string> PorytilesTilesetRepoImpl::load(const std::string &name) {
    // TODO : impl
    Panic("unimplemented");
}

} // namespace porytiles

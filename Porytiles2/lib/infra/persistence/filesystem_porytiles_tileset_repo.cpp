#include <porytiles2/infra/persistence/filesystem_porytiles_tileset_repo.hpp>

#include <expected>
#include <memory>
#include <string>

#include <porytiles2/domain/aggregates/porytiles_tileset.hpp>
#include <porytiles2/templates/panic.hpp>
#include <porytiles2/templates/result.hpp>

namespace porytiles {

void FilesystemPorytilesTilesetRepo::save(const PorytilesTileset &tileset) {
    // TODO : impl
    Panic("unimplemented");
}

Result<std::unique_ptr<PorytilesTileset>> FilesystemPorytilesTilesetRepo::load(const std::string &name) {
    // TODO : impl
    Panic("unimplemented");
}

} // namespace porytiles

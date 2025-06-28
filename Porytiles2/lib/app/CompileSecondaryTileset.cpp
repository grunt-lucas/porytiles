#include "porytiles2/app/CompileSecondaryTileset.hpp"

#include <expected>
#include <memory>

#include "porytiles2/domain/services/TilesetCompilerService.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

Result<void>
CompileSecondaryTileset::Compile(const std::string &tileset) const {
  Panic("unimplemented");
}

} // namespace porytiles

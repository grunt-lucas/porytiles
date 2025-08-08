#include "porytiles2/infra/repos/project_tileset_key_provider.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

static const std::filesystem::path kPrimaryTilesetsRelativePath =
    std::filesystem::path{"data"} / "tilesets" / "primary";
static const std::filesystem::path kSecondaryTilesetsRelativePath =
    std::filesystem::path{"data"} / "tilesets" / "secondary";

std::any ProjectTilesetKeyProvider::key_for(const std::string &tileset_name, const TilesetArtifact &artifact) const {
    panic("TODO: unimplemented");
}

} // namespace porytiles2

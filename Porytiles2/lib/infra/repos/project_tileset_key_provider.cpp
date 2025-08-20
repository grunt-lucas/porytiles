#include "porytiles2/infra/repos/project_tileset_key_provider.hpp"

#include <filesystem>
#include <string>

#include "fmt/format.h"

#include "porytiles2/templates/panic.hpp"

namespace {

const std::filesystem::path kPrimaryTilesetsRelativePath = std::filesystem::path{"data"} / "tilesets" / "primary";
const std::filesystem::path kSecondaryTilesetsRelativePath = std::filesystem::path{"data"} / "tilesets" / "secondary";

std::filesystem::path get_tileset_path(const std::string &tileset_name, const std::filesystem::path &project_root) {
    if (std::filesystem::exists(project_root / kPrimaryTilesetsRelativePath / tileset_name)) {
        return project_root / kPrimaryTilesetsRelativePath / tileset_name;
    }
    if (std::filesystem::exists(project_root / kSecondaryTilesetsRelativePath / tileset_name)) {
        return project_root / kSecondaryTilesetsRelativePath / tileset_name;
    }
    porytiles2::panic(fmt::format("tileset {} does not exist", tileset_name));
}

} // namespace

namespace porytiles2 {

static const std::filesystem::path porytiles_directory{"porytiles"};
static const std::filesystem::path bottom_png{"bottom.png"};
static const std::filesystem::path middle_png{"middle.png"};
static const std::filesystem::path top_png{"top.png"};
static const std::filesystem::path attributes_csv{"attributes.csv"};
static const std::filesystem::path anim{"anim"};

ArtifactKey ProjectTilesetKeyProvider::key_for(const std::string &tileset_name, const TilesetArtifact &artifact) const {
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);

    switch (artifact.type()) {
    case TilesetArtifact::Type::bottom_png:
        return ArtifactKey{tileset_path / porytiles_directory / bottom_png};
    case TilesetArtifact::Type::middle_png:
        return ArtifactKey{tileset_path / porytiles_directory / middle_png};
    case TilesetArtifact::Type::top_png:
        return ArtifactKey{tileset_path / porytiles_directory / top_png};
    case TilesetArtifact::Type::attributes_csv:
        return ArtifactKey{tileset_path / porytiles_directory / attributes_csv};
    case TilesetArtifact::Type::porytiles_anim_frame: {
        if (!artifact.name().has_value()) {
            panic("missing porytiles anim frame name");
        }
        if (!artifact.index().has_value()) {
            panic("missing porytiles anim frame index");
        }
        const auto anim_name = artifact.name().value();
        const auto frame_num = artifact.index().value();
        return ArtifactKey{tileset_path / porytiles_directory / anim / anim_name / fmt::format("{}.png", frame_num)};
    }
    default:
        panic("unhandled TilesetArtifact::Type");
    }
}

std::set<std::string> ProjectTilesetKeyProvider::discover_porytiles_anims(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

std::set<int> ProjectTilesetKeyProvider::discover_porytiles_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const {
    panic("TODO: unimplemented");
}

std::set<std::string> ProjectTilesetKeyProvider::discover_porymap_anims(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

std::set<int> ProjectTilesetKeyProvider::discover_porymap_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const {
    panic("TODO: unimplemented");
}

bool ProjectTilesetKeyProvider::exists(const ArtifactKey &key) const {
    panic("TODO: unimplemented");
}

bool ProjectTilesetKeyProvider::tileset_exists(const std::string &tileset_name) const {
    if (std::filesystem::exists(project_root_ / kPrimaryTilesetsRelativePath / tileset_name)) {
        return true;
    }
    if (std::filesystem::exists(project_root_ / kSecondaryTilesetsRelativePath / tileset_name)) {
        return true;
    }
    return false;
}

} // namespace porytiles2

#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"

#include <filesystem>
#include <string>

#include "fmt/format.h"

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

const std::filesystem::path primary_tileset_rel_path = std::filesystem::path{"data"} / "tilesets" / "primary";
const std::filesystem::path secondary_tileset_rel_path = std::filesystem::path{"data"} / "tilesets" / "secondary";

std::filesystem::path get_tileset_path(const std::string &tileset_name, const std::filesystem::path &project_root)
{
    if (std::filesystem::exists(project_root / primary_tileset_rel_path / tileset_name)) {
        return project_root / primary_tileset_rel_path / tileset_name;
    }
    if (std::filesystem::exists(project_root / secondary_tileset_rel_path / tileset_name)) {
        return project_root / secondary_tileset_rel_path / tileset_name;
    }
    panic(fmt::format("tileset '{}' does not exist", tileset_name));
}

const std::filesystem::path metatiles_bin{"metatiles.bin"};
const std::filesystem::path metatile_attributes_bin{"metatile_attributes.bin"};
const std::filesystem::path tiles_png{"tiles.png"};
const std::filesystem::path porymap_pals{"palettes"};
const std::filesystem::path anim{"anim"};

const std::filesystem::path porytiles_directory{"porytiles"};
const std::filesystem::path bottom_png{"bottom.png"};
const std::filesystem::path middle_png{"middle.png"};
const std::filesystem::path top_png{"top.png"};
const std::filesystem::path attributes_csv{"attributes.csv"};
const std::filesystem::path porytiles_pals{"palettes"};
const std::filesystem::path config{"porytiles.yaml"};
const std::filesystem::path local_config{"porytiles.local.yaml"};

} // namespace

namespace porytiles2 {

/*
 * Porymap artifacts
 */
ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_metatiles_bin(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / metatiles_bin};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_metatile_attributes_bin(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / metatile_attributes_bin};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_tiles_png(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / tiles_png};
}

ArtifactKey
ProjectTilesetArtifactKeyProvider::key_for_porymap_pal_n(const std::string &tileset_name, std::size_t index) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porymap_pals / (pal_filename(index))};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, std::size_t frame_index) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / anim / anim_name / (std::to_string(frame_index) + std::string{".png"})};
}

[[nodiscard]] ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_key_frame(
    const std::string &tileset_name, const std::string &anim_name) const
{
    panic("TODO: implement");
}

[[nodiscard]] ArtifactKey
ProjectTilesetArtifactKeyProvider::key_for_generated_anim_code(const std::string &tileset_name) const
{
    panic("TODO: implement");
}

/*
 * Porytiles artifacts
 */
ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_bottom_png(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porytiles_directory / bottom_png};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_middle_png(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porytiles_directory / middle_png};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_top_png(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porytiles_directory / top_png};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_attributes_csv(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porytiles_directory / attributes_csv};
}

ArtifactKey
ProjectTilesetArtifactKeyProvider::key_for_porytiles_pal_n(const std::string &tileset_name, std::size_t index) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porytiles_directory / porytiles_pals / (pal_filename(index))};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, std::size_t frame_index) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{
        tileset_path / porytiles_directory / anim / anim_name / (pad_two_digits(frame_index) + std::string{".png"})};
}

[[nodiscard]] ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_key_frame(
    const std::string &tileset_name, const std::string &anim_name) const
{
    panic("TODO: implement");
}

[[nodiscard]] ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_anim_yaml(const std::string &tileset_name) const
{
    panic("TODO: implement");
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_config(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porytiles_directory / config};
}

ArtifactKey ProjectTilesetArtifactKeyProvider::key_for_local_config(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    return ArtifactKey{tileset_path / porytiles_directory / local_config};
}

bool ProjectTilesetArtifactKeyProvider::artifact_exists(const ArtifactKey &key) const
{
    const std::filesystem::path artifact{key.key()};
    return std::filesystem::exists(artifact);
}

bool ProjectTilesetArtifactKeyProvider::tileset_exists(const std::string &tileset_name) const
{
    if (std::filesystem::exists(project_root_ / primary_tileset_rel_path / tileset_name)) {
        return true;
    }
    if (std::filesystem::exists(project_root_ / secondary_tileset_rel_path / tileset_name)) {
        return true;
    }
    return false;
}

std::set<std::string> ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    const auto anims_dir = tileset_path / porytiles_directory / anim;

    std::set<std::string> anim_names;

    if (!std::filesystem::exists(anims_dir) || !std::filesystem::is_directory(anims_dir)) {
        return anim_names;
    }

    for (const auto &entry : std::filesystem::directory_iterator(anims_dir)) {
        if (!entry.is_directory()) {
            // TODO: warn user about stray file in porytiles/anim folder?
            continue;
        }

        // Check if 00.png exists (required for Porytiles animations)
        const auto frame_00_path = entry.path() / "00.png";
        if (!std::filesystem::exists(frame_00_path)) {
            // TODO: this is an error condition, an anim folder with no 00.png is invalid
            continue;
        }

        const auto anim_name = entry.path().filename().string();
        anim_names.insert(anim_name);
    }

    return anim_names;
}

std::set<int> ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    const auto anim_dir = tileset_path / porytiles_directory / anim / anim_name;

    std::set<int> frame_indices;

    if (!std::filesystem::exists(anim_dir) || !std::filesystem::is_directory(anim_dir)) {
        return frame_indices;
    }

    for (const auto &entry : std::filesystem::directory_iterator(anim_dir)) {
        if (!entry.is_regular_file()) {
            // TODO: warn user about stray folder in porytiles/anim/anim_name folder
            continue;
        }

        const auto filename = entry.path().filename().string();

        if (filename.length() != 6 || !filename.ends_with(".png")) {
            // TODO: warn user about stray file in porytiles/anim/anim_name folder
            continue;
        }

        // Skip 00.png (frame 0 is required, not discovered), handled in the main discover_anims method
        if (filename == "00.png") {
            continue;
        }

        // Check if it's a valid two-digit number
        const auto frame_str = filename.substr(0, 2);
        if (!std::isdigit(frame_str[0]) || !std::isdigit(frame_str[1])) {
            // TODO: warn user about stray file in porytiles/anim/anim_name folder
            continue;
        }
        const int frame_index = std::stoi(frame_str);
        frame_indices.insert(frame_index);
    }

    return frame_indices;
}

std::set<std::string> ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const std::string &tileset_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    const auto anims_dir = tileset_path / anim;

    std::set<std::string> anim_names;

    if (!std::filesystem::exists(anims_dir) || !std::filesystem::is_directory(anims_dir)) {
        return anim_names;
    }

    for (const auto &entry : std::filesystem::directory_iterator(anims_dir)) {
        if (!entry.is_directory()) {
            // TODO: warn user about stray file in anim folder?
            continue;
        }

        const auto anim_name = entry.path().filename().string();

        // Check if 00.png exists (required for Porymap animations)
        const auto frame_0_path = entry.path() / "0.png";
        if (!std::filesystem::exists(frame_0_path)) {
            diag_->warn(
                "missing-required-artifact",
                format_->format("missing required artifact: '{}'", FormatParam{frame_0_path.string(), Style::bold}));
            diag_->note(
                "missing-required-artifact",
                format_->format("skipping registry of Porymap anim '{}'", FormatParam{anim_name, Style::bold}));
            continue;
        }

        anim_names.insert(anim_name);
    }

    return anim_names;
}

std::set<int> ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    const auto tileset_path = get_tileset_path(tileset_name, project_root_);
    const auto anim_dir = tileset_path / anim / anim_name;

    std::set<int> frame_indices;

    if (!std::filesystem::exists(anim_dir) || !std::filesystem::is_directory(anim_dir)) {
        return frame_indices;
    }

    for (const auto &entry : std::filesystem::directory_iterator(anim_dir)) {
        if (!entry.is_regular_file()) {
            // TODO: warn user about stray folder in anim/anim_name folder
            continue;
        }

        const auto filename = entry.path().filename().string();

        if (filename.length() != 5 || !filename.ends_with(".png")) {
            // TODO: warn user about stray file in porytiles/anim/anim_name folder
            continue;
        }

        // Skip 0.png (frame 0 is required, not discovered), handled in the main discover_anims method
        if (filename == "0.png") {
            continue;
        }

        // Check if it's a valid one-digit number
        const auto frame_str = filename.substr(0, 1);
        if (!std::isdigit(frame_str[0])) {
            // TODO: warn user about stray file in anim/anim_name folder
            continue;
        }
        const int frame_index = std::stoi(frame_str);
        frame_indices.insert(frame_index);
    }

    return frame_indices;
}

[[nodiscard]] std::filesystem::path
ProjectTilesetArtifactKeyProvider::tileset_root(const std::string &tileset_name) const
{
    return get_tileset_path(tileset_name, project_root_);
}

} // namespace porytiles2

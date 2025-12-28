#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <string>

#include "porytiles2/domain/models/tileset_name.hpp"
#include "porytiles2/infra/repos/tileset_artifact_paths.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

const std::filesystem::path anim{"anim"};
const std::filesystem::path include{"include"};
const std::filesystem::path generated_anim_code_header{"generated_anim_code.h"};

const std::filesystem::path porytiles_directory{"porytiles"};
const std::filesystem::path bottom_png{"bottom.png"};
const std::filesystem::path middle_png{"middle.png"};
const std::filesystem::path top_png{"top.png"};
const std::filesystem::path attributes_csv{"attributes.csv"};
const std::filesystem::path porytiles_pals{"palettes"};
const std::filesystem::path anim_yaml{"anim.yaml"};
const std::filesystem::path key_frame{"key.png"};
const std::filesystem::path config{"porytiles.yaml"};
const std::filesystem::path local_config{"porytiles.local.yaml"};

} // namespace

namespace porytiles2 {

/*
 * Porymap artifacts
 */
ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_metatiles_bin(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format("failed to get metatiles.bin key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.metatiles_path()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatile_attributes_bin(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format(
            "failed to get metatile_attributes.bin key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.metatile_attributes_path()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_tiles_png(const TilesetName &name) const
{
    /*
     * TODO: instead of harcoding "tiles.png" here, we should extract the filename from the INCBIN and replace .4bpp
     * extension with .png. This would be a cleaner way to handle things, and could handle a case where the user changed
     * the name of the tiles file.
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format("failed to get tiles.png key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.tiles_path().parent_path() / "tiles.png"};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_pal_n(const TilesetName &name, std::size_t index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        paths,
        artifact_paths(name),
        format_->format("failed to get Porymap pal key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{project_root_ / paths.palettes_dir() / pal_filename(index)};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(
    const TilesetName &name, const std::string &anim_name, std::size_t frame_index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format(
            "failed to get Porymap anim frame key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / anim / anim_name / (std::to_string(frame_index) + std::string{".png"})};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_generated_anim_code(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format(
            "failed to get generated anim code key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / include / generated_anim_code_header};
}

/*
 * Porytiles artifacts
 */
ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_bottom_png(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get bottom.png key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / bottom_png};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_middle_png(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get middle.png key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / middle_png};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_top_png(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get top.png key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / top_png};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_attributes_csv(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format(
            "failed to get attributes.csv key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / attributes_csv};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_pal_n(const TilesetName &name, std::size_t index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get Porytiles pal key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / porytiles_pals / pal_filename(index)};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(
    const TilesetName &name, const std::string &anim_name, std::size_t frame_index) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format(
            "failed to get Porytiles anim frame key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{
        tileset_path / porytiles_directory / anim / anim_name / (std::to_string(frame_index) + std::string{".png"})};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_key_frame(
    const TilesetName &name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format(
            "failed to get Porytiles anim key frame key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / anim / anim_name / key_frame};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_anim_yaml(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get anim.yaml key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / anim / anim_yaml};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_config(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get config key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / config};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_local_config(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format("failed to get local config key for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        ArtifactKey);
    return ArtifactKey{tileset_path / porytiles_directory / local_config};
}

bool ProjectTilesetArtifactKeyProvider::artifact_exists(const ArtifactKey &key) const
{
    const std::filesystem::path artifact{key.key()};
    return std::filesystem::exists(artifact);
}

bool ProjectTilesetArtifactKeyProvider::tileset_exists(const TilesetName &name) const
{
    const auto exists_result = metadata_provider_->tileset_exists(name);
    return exists_result.has_value() && exists_result.value();
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format(
            "failed to discover Porytiles anims for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        std::set<std::string>);
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

        // Check if key frame exists (required for Porytiles animations)
        const auto key_frame_path = entry.path() / key_frame;
        if (!std::filesystem::exists(key_frame_path)) {
            // TODO: this is an error condition, an anim folder with no key.png is invalid
            continue;
        }

        // Check if 0.png exists (required for Porytiles animations)
        const auto frame_0_path = entry.path() / "0.png";
        if (!std::filesystem::exists(frame_0_path)) {
            // TODO: this is an error condition, an anim folder with no 0.png is invalid
            continue;
        }

        const auto anim_name = entry.path().filename().string();
        anim_names.insert(anim_name);
    }

    return anim_names;
}

ChainableResult<std::set<int>> ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(
    const TilesetName &name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset_path,
        tileset_root(name),
        format_->format(
            "failed to discover Porytiles anim frames for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        std::set<int>);
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

        if (!filename.ends_with(".png")) {
            // TODO: warn user about stray file in porytiles/anim/anim_name folder
            continue;
        }

        // Skip 00.png (frame 0 is required, not discovered), handled in the main discover_anims method
        if (filename == "0.png") {
            continue;
        }

        // Check if it's a valid number
        const auto frame_str = filename.substr(0, filename.size() - 4); // strip ".png"
        if (!std::ranges::all_of(frame_str, ::isdigit)) {
            // TODO: warn user about stray file in porytiles/anim/anim_name folder
            continue;
        }
        const int frame_index = std::stoi(frame_str);
        frame_indices.insert(frame_index);
    }

    return frame_indices;
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const TilesetName &name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        frame_paths,
        metadata_provider_->animation_frame_paths_for(name),
        format_->format(
            "failed to discover Porymap anims for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        std::set<std::string>);

    std::set<std::string> anim_names;
    for (const auto &anim_name : frame_paths | std::views::keys) {
        anim_names.insert(anim_name);
    }
    return anim_names;
}

ChainableResult<std::set<int>> ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(
    const TilesetName &name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        frame_paths,
        metadata_provider_->animation_frame_paths_for(name),
        format_->format(
            "failed to discover Porymap anim frames for tileset '{}'", FormatParam{name.shorthand(), Style::bold}),
        std::set<int>);

    std::set<int> frame_indices;

    const auto it = frame_paths.find(anim_name);
    if (it == frame_paths.end()) {
        return frame_indices;
    }

    const auto &frames = it->second;
    // Skip frame 0 (required, not discovered) - preserves existing semantics
    for (std::size_t i = 1; i < frames.size(); ++i) {
        frame_indices.insert(static_cast<int>(i));
    }

    return frame_indices;
}

ChainableResult<TilesetArtifactPaths> ProjectTilesetArtifactKeyProvider::artifact_paths(const TilesetName &name) const
{
    auto paths_result = metadata_provider_->artifact_paths_for(name);
    if (!paths_result.has_value()) {
        /*
         * NOTE: std::move required here because ChainableResult's passthrough constructor is templated. When source and
         * destination types match, C++ prefers the deleted copy constructor over the template. Using std::move invokes
         * the (defaulted) move constructor instead.
         *
         * NOTE: since this calls the defaulted move ctor instead of the passthrough templated ctor, technically we
         * don't get an additional error chain link added here. That's probably fine, since it would just be a blank
         * FormattableError{}, which will be skipped by the UserDiagnostic fatal chain printer regardless. However, if
         * we ever added code that counted error chain links, we should note that this workaround could cause the link
         * count to differ from expectations.
         */
        return ChainableResult{std::move(paths_result)};
    }
    return paths_result.value();
}

[[nodiscard]] ChainableResult<std::filesystem::path>
ProjectTilesetArtifactKeyProvider::tileset_root(const TilesetName &name) const
{
    const auto paths_result = artifact_paths(name);
    if (!paths_result.has_value()) {
        return ChainableResult<std::filesystem::path>{paths_result};
    }

    return project_root_ / paths_result.value().tileset_root();
}

ChainableResult<std::optional<AnimationCallbackInfo>>
ProjectTilesetArtifactKeyProvider::animation_callback_info_for(const TilesetName &name) const
{
    // Delegate to metadata provider
    return metadata_provider_->animation_callback_info_for(name);
}

} // namespace porytiles2

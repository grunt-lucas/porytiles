#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"

#include <filesystem>
#include <format>
#include <set>
#include <string>

#include "porytiles2/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles2/utilities/dynamic_cased_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles2;

// Artifact paths
const std::filesystem::path src_dir{"src"};
const std::filesystem::path porytiles_generated_include{"include/porytiles_generated/tilesets"};
const std::filesystem::path porytiles_src{"porytiles_src"};
const std::filesystem::path porytiles_bin{"porytiles_bin"};
const std::filesystem::path anim_dir{"anim"};
const std::filesystem::path generated_anim_code_header{"generated_anim_code.h"};
const std::filesystem::path bottom_png{"bottom.png"};
const std::filesystem::path middle_png{"middle.png"};
const std::filesystem::path top_png{"top.png"};
const std::filesystem::path attributes_csv{"attributes.csv"};
const std::filesystem::path porytiles_pals{"palettes"};
const std::filesystem::path porymap_pals{"palettes"};
const std::filesystem::path anim_yaml{"anim.yaml"};
const std::filesystem::path metatiles_bin{"metatiles.bin"};
const std::filesystem::path attrs_bin{"metatile_attributes.bin"};
const std::filesystem::path tiles_png{"tiles.png"};

/**
 * @brief Scans a directory for subdirectories and validates their names are snake_case.
 *
 * @details
 * Iterates through a directory and collects all subdirectory names. If any subdirectory
 * name is not in valid snake_case format, returns an error with a helpful message.
 * If the directory doesn't exist, returns an empty set (not an error).
 *
 * @param dir_path The directory to scan for subdirectories
 * @param artifact_type Human-readable description for error messages (e.g., "animation", "frame")
 * @param format TextFormatter for styled error messages
 * @return A set of subdirectory names, or error if any name fails snake_case validation
 */
[[nodiscard]] ChainableResult<std::set<std::string>> scan_subdirectories(
    const std::filesystem::path &dir_path, const std::string &artifact_type, const TextFormatter *format)
{
    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        return std::set<std::string>{};
    }

    std::set<std::string> result;
    for (const auto &entry : std::filesystem::directory_iterator{dir_path}) {
        if (!entry.is_directory()) {
            continue;
        }
        const std::string dir_name = entry.path().filename().string();
        const auto expected_snake = DynamicCasedName{dir_name}.to_snake_case();
        if (expected_snake != dir_name) {
            return ChainableResult<std::set<std::string>>{FormattableError{format->format(
                "{} directory name '{}' must be snake_case (expected '{}')",
                FormatParam{artifact_type, Style::bold},
                FormatParam{dir_name, Style::bold},
                FormatParam{expected_snake, Style::bold})}};
        }
        result.insert(dir_name);
    }
    return result;
}

/**
 * @brief Scans a directory for PNG files and validates their names are snake_case.
 *
 * @details
 * Iterates through a directory and collects all PNG file stems (filenames without .png extension).
 * If any file stem is not in valid snake_case format, returns an error with a helpful message.
 * If the directory doesn't exist, returns an empty set (not an error).
 *
 * @param dir_path The directory to scan for PNG files
 * @param artifact_type Human-readable description for error messages (e.g., "frame")
 * @param format TextFormatter for styled error messages
 * @return A set of PNG file stems, or error if any name fails snake_case validation
 */
[[nodiscard]] ChainableResult<std::set<std::string>>
scan_png_files(const std::filesystem::path &dir_path, const std::string &artifact_type, const TextFormatter *format)
{
    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        return std::set<std::string>{};
    }

    std::set<std::string> result;
    for (const auto &entry : std::filesystem::directory_iterator{dir_path}) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto &path = entry.path();
        if (path.extension() != ".png") {
            continue;
        }
        const std::string stem = path.stem().string();
        const auto expected_snake = DynamicCasedName{stem}.to_snake_case();
        if (expected_snake != stem) {
            return ChainableResult<std::set<std::string>>{FormattableError{format->format(
                "{} file name '{}' must be snake_case (expected '{}.png')",
                FormatParam{artifact_type, Style::bold},
                FormatParam{path.filename().string(), Style::bold},
                FormatParam{expected_snake, Style::bold})}};
        }
        result.insert(stem);
    }
    return result;
}

} // namespace

namespace porytiles2 {

/*
 * Porymap artifacts
 */
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatiles_bin(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path =
        std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_bin / metatiles_bin;

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatile_attributes_bin(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path =
        std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_bin / attrs_bin;

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_tiles_png(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path =
        std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_bin / tiles_png;

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_pal_n(const std::string &tileset_name, std::size_t index) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path = std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_bin /
                                 porymap_pals / pal_filename(index);

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path = std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_bin /
                                 anim_dir / anim_name / (frame_name + std::string{".png"});

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_params(const std::string &tileset_name) const
{
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path = porytiles_generated_include / snake_tileset_dir / generated_anim_code_header;
    return ArtifactKey{path.string()};
}

/*
 * Porytiles artifacts
 */
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_bottom_png(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path =
        std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src / bottom_png;

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_middle_png(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path =
        std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src / middle_png;

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_top_png(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path = std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src / top_png;

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_attributes_csv(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path =
        std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src / attributes_csv;

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_pal_n(const std::string &tileset_name, std::size_t index) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path = std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src /
                                 porytiles_pals / pal_filename(index);

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path = std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src /
                                 anim_dir / anim_name / (frame_name + std::string{".png"});

    return ArtifactKey{path.string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_params(const std::string &tileset_name) const
{
    // Get primary/secondary status of tileset
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();

    // Get base path from config
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, ArtifactKey);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, ArtifactKey);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();
    std::filesystem::path path =
        std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src / anim_dir / anim_yaml;

    return ArtifactKey{path.string()};
}

bool ProjectTilesetArtifactKeyProvider::artifact_exists(const ArtifactKey &key) const
{
    // Keys are relative to project_root_, so prepend for filesystem operations
    const std::filesystem::path artifact = project_root_ / key.key();
    return std::filesystem::exists(artifact);
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porymap_anims(const std::string &tileset_name) const
{
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, std::set<std::string>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, std::set<std::string>);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();

    const std::filesystem::path anim_path =
        project_root_ / std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_bin / anim_dir;

    return scan_subdirectories(anim_path, "Porymap animation", format_);
}

ChainableResult<std::set<std::string>> ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_bin, tileset_name, std::set<std::string>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_bin, tileset_name, std::set<std::string>);
    auto base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();

    const std::filesystem::path frame_dir = project_root_ / std::filesystem::path{base_path.value()} /
                                            snake_tileset_dir / porytiles_bin / anim_dir / anim_name;

    return scan_png_files(frame_dir, "Porymap animation frame", format_);
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) const
{
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_src, tileset_name, std::set<std::string>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_src, tileset_name, std::set<std::string>);
    auto base_path = is_secondary ? tileset_paths_secondary_src : tileset_paths_primary_src;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();

    const std::filesystem::path anim_path =
        project_root_ / std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src / anim_dir;

    return scan_subdirectories(anim_path, "Porytiles animation", format_);
}

ChainableResult<std::set<std::string>> ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    const bool is_secondary = metadata_provider_.is_secondary(tileset_name).value();
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_secondary_src, tileset_name, std::set<std::string>);
    PT_UNWRAP_TILESET_CONFIG_PTR(config_, tileset_paths_primary_src, tileset_name, std::set<std::string>);
    auto base_path = is_secondary ? tileset_paths_secondary_src : tileset_paths_primary_src;
    const std::string snake_tileset_dir = DynamicCasedName{extract_tileset_shorthand(tileset_name)}.to_snake_case();

    const std::filesystem::path frame_dir = project_root_ / std::filesystem::path{base_path.value()} /
                                            snake_tileset_dir / porytiles_src / anim_dir / anim_name;

    return scan_png_files(frame_dir, "Porytiles animation frame", format_);
}

} // namespace porytiles2

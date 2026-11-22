#include "porytiles/infra/repos/project_tileset_artifact_key_provider.hpp"

#include <filesystem>
#include <format>
#include <set>
#include <string>
#include <string_view>

#include "porytiles/utilities/dynamic_cased_name.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace {

using namespace porytiles;

// Artifact path components
constexpr std::string_view porytiles_generated_include{"include/porytiles_generated/tilesets"};
constexpr std::string_view porytiles_src{"porytiles_src"};
constexpr std::string_view porytiles_bin{"porytiles_bin"};
constexpr std::string_view anim_dir{"anim"};
constexpr std::string_view generated_anim_code_header{"generated_anim_code.h"};
constexpr std::string_view bottom_png{"bottom.png"};
constexpr std::string_view middle_png{"middle.png"};
constexpr std::string_view top_png{"top.png"};
constexpr std::string_view attributes_csv{"attributes.csv"};
constexpr std::string_view porytiles_palettes{"palettes"};
constexpr std::string_view porymap_palettes{"palettes"};
constexpr std::string_view anim_json{"anim.json"};
constexpr std::string_view metatiles_bin{"metatiles.bin"};
constexpr std::string_view attributes_bin{"metatile_attributes.bin"};
constexpr std::string_view tiles_png{"tiles.png"};

/// @brief Scans a directory for subdirectories and validates their names are snake_case.
///
/// @details
/// Iterates through a directory and collects all subdirectory names. If any subdirectory
/// name is not in valid snake_case format, returns an error with a helpful message.
/// If the directory doesn't exist, returns an empty set (not an error).
///
/// @param dir_path The directory to scan for subdirectories
/// @param artifact_type Human-readable description for error messages (e.g., "animation", "frame")
/// @param format TextFormatter for styled error messages
/// @return A set of subdirectory names, or error if any name fails snake_case validation
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

/// @brief Scans a directory for PNG files and validates their names are snake_case.
///
/// @details
/// Iterates through a directory and collects all PNG file stems (filenames without .png extension).
/// If any file stem is not in valid snake_case format, returns an error with a helpful message.
/// If the directory doesn't exist, returns an empty set (not an error).
///
/// @param dir_path The directory to scan for PNG files
/// @param artifact_type Human-readable description for error messages (e.g., "frame")
/// @param format TextFormatter for styled error messages
/// @return A set of PNG file stems, or error if any name fails snake_case validation
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

/// @brief The asset directories a tileset's artifacts are split across.
enum class TilesetAssetDir { porytiles_src, porytiles_bin };

/// @brief Resolves one of a tileset's asset directories, relative to the project root, accounting for primary/secondary
/// split.
[[nodiscard]] ChainableResult<std::filesystem::path> resolve_asset_dir(
    const InfraConfig *infra_config,
    const TilesetMetadataProvider *metadata_provider,
    const std::string &tileset_name,
    TilesetAssetDir asset_dir)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        is_secondary,
        metadata_provider->is_secondary(tileset_name),
        std::filesystem::path,
        "Failed to determine primary/secondary status.");
    const std::string snake_tileset_dir = extract_tileset_cased_name(tileset_name).to_snake_case();

    switch (asset_dir) {
    case TilesetAssetDir::porytiles_src: {
        PT_UNWRAP_TILESET_CONFIG_PTR(infra_config, tileset_paths_secondary_src, tileset_name, std::filesystem::path);
        PT_UNWRAP_TILESET_CONFIG_PTR(infra_config, tileset_paths_primary_src, tileset_name, std::filesystem::path);
        const auto &base_path = is_secondary ? tileset_paths_secondary_src : tileset_paths_primary_src;
        return std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_src;
    }
    case TilesetAssetDir::porytiles_bin: {
        PT_UNWRAP_TILESET_CONFIG_PTR(infra_config, tileset_paths_secondary_bin, tileset_name, std::filesystem::path);
        PT_UNWRAP_TILESET_CONFIG_PTR(infra_config, tileset_paths_primary_bin, tileset_name, std::filesystem::path);
        const auto &base_path = is_secondary ? tileset_paths_secondary_bin : tileset_paths_primary_bin;
        return std::filesystem::path{base_path.value()} / snake_tileset_dir / porytiles_bin;
    }
    }
    panic("resolve_asset_dir: unhandled TilesetAssetDir");
}

} // namespace

namespace porytiles {

// Porymap artifacts
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatiles_bin(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_bin), ArtifactKey);
    return ArtifactKey{(dir / metatiles_bin).string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_metatile_attributes_bin(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_bin), ArtifactKey);
    return ArtifactKey{(dir / attributes_bin).string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_tiles_png(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_bin), ArtifactKey);
    return ArtifactKey{(dir / tiles_png).string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_palette_n(const std::string &tileset_name, std::size_t index) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_bin), ArtifactKey);
    return ArtifactKey{(dir / porymap_palettes / palette_filename(index)).string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_bin), ArtifactKey);
    return ArtifactKey{(dir / anim_dir / anim_name / (frame_name + std::string{".png"})).string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porymap_anim_params(const std::string &tileset_name) const
{
    const std::string snake_tileset_dir = extract_tileset_cased_name(tileset_name).to_snake_case();
    const std::filesystem::path path =
        std::filesystem::path{porytiles_generated_include} / snake_tileset_dir / generated_anim_code_header;
    return ArtifactKey{path.string()};
}

// Porytiles artifacts
ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_bottom_png(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src), ArtifactKey);
    return ArtifactKey{(dir / bottom_png).string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_middle_png(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src), ArtifactKey);
    return ArtifactKey{(dir / middle_png).string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_top_png(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src), ArtifactKey);
    return ArtifactKey{(dir / top_png).string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_attributes_csv(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src), ArtifactKey);
    return ArtifactKey{(dir / attributes_csv).string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_palette_n(const std::string &tileset_name, std::size_t index) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src), ArtifactKey);
    return ArtifactKey{(dir / porytiles_palettes / palette_filename(index)).string()};
}

ChainableResult<ArtifactKey> ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_frame(
    const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src), ArtifactKey);
    return ArtifactKey{(dir / anim_dir / anim_name / (frame_name + std::string{".png"})).string()};
}

ChainableResult<ArtifactKey>
ProjectTilesetArtifactKeyProvider::key_for_porytiles_anim_params(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir, resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src), ArtifactKey);
    return ArtifactKey{(dir / anim_dir / anim_json).string()};
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
    PT_TRY_ASSIGN_PASS_ERR(
        dir,
        resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_bin),
        std::set<std::string>);
    return scan_subdirectories(project_root_ / dir / anim_dir, "Porymap animation", format_);
}

ChainableResult<std::set<std::string>> ProjectTilesetArtifactKeyProvider::discover_porymap_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir,
        resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_bin),
        std::set<std::string>);
    return scan_png_files(project_root_ / dir / anim_dir / anim_name, "Porymap animation frame", format_);
}

ChainableResult<std::set<std::string>>
ProjectTilesetArtifactKeyProvider::discover_porytiles_anims(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir,
        resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src),
        std::set<std::string>);
    return scan_subdirectories(project_root_ / dir / anim_dir, "Porytiles animation", format_);
}

ChainableResult<std::set<std::string>> ProjectTilesetArtifactKeyProvider::discover_porytiles_anim_frames(
    const std::string &tileset_name, const std::string &anim_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        dir,
        resolve_asset_dir(config_, metadata_provider_, tileset_name, TilesetAssetDir::porytiles_src),
        std::set<std::string>);
    return scan_png_files(project_root_ / dir / anim_dir / anim_name, "Porytiles animation frame", format_);
}

} // namespace porytiles

#include "porytiles/infra/repos/project_tileset_artifact_writer.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <ranges>
#include <sstream>
#include <string>

#include "porytiles/domain/config/role_pin_definition.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/infra/algorithms/porymap_artifact_parsers.hpp"
#include "porytiles/infra/services/png_indexed_image_saver.hpp"
#include "porytiles/infra/services/png_rgba_image_saver.hpp"
#include "porytiles/utilities/filesystem_utils.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/result/error.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace {

using namespace porytiles;

// Marker directories for artifact categorization
const std::filesystem::path porytiles_src_marker{"porytiles_src"};
const std::filesystem::path porytiles_bin_marker{"porytiles_bin"};
const std::filesystem::path porytiles_generated_marker{"porytiles_generated"};

/// @brief Categories for artifact destinations.
///
/// @details
/// Artifacts are categorized to determine how they should be atomically committed:
/// - porytiles_src: Porytiles-format source assets (bottom.png, middle.png, etc.)
/// - porytiles_bin: Porymap-format binary assets (metatiles.bin, tiles.png, etc.)
/// - special: Files that live outside standard directories (generated_anim_code.h)
enum class ArtifactCategory { porytiles_src, porytiles_bin, special };

/// @brief Parsed information about an artifact's destination.
///
/// @details
/// Contains the category and the directory path that should be atomically moved. For porytiles_src/porytiles_bin
/// categories, `directory` is the path up to and including the marker directory. For special files, `directory` is the
/// parent directory of the file.
struct ArtifactPathInfo {
    ArtifactCategory category;
    std::filesystem::path directory; ///< The directory to atomically move
};

/// @brief Categorizes an artifact key to determine its commit handling.
///
/// @details
/// Parses the artifact key path to find marker directories (porytiles_src, porytiles_bin, or porytiles_generated) and
/// returns categorization info. The directory returned is the path up to and including the marker, which will be
/// atomically moved during commit.
///
/// @param key_path The artifact key path (relative to project root)
/// @return ArtifactPathInfo with category and directory to move
[[nodiscard]] ArtifactPathInfo categorize_artifact_key(const std::filesystem::path &key_path)
{
    // Walk through path components to find marker directories
    std::filesystem::path accumulated;
    for (const auto &component : key_path) {
        accumulated /= component;

        if (component == porytiles_src_marker) {
            return ArtifactPathInfo{ArtifactCategory::porytiles_src, accumulated};
        }
        if (component == porytiles_bin_marker) {
            return ArtifactPathInfo{ArtifactCategory::porytiles_bin, accumulated};
        }
        if (component == porytiles_generated_marker) {
            // For porytiles_generated, the whole path up to the file's parent is special
            return ArtifactPathInfo{ArtifactCategory::special, key_path.parent_path()};
        }
    }

    // Default to special category for unrecognized paths
    return ArtifactPathInfo{ArtifactCategory::special, key_path.parent_path()};
}

/// @brief Creates a unique temporary directory inside the project root.
///
/// @details
/// Creates a tmpdir at {project_root}/.porytiles_tmp_{random_hex} to ensure
/// same-filesystem with project files, enabling atomic std::filesystem::rename().
///
/// @param project_root The project root directory
/// @return Path to the newly created temporary directory
/// @post The returned path points to an existing, empty directory
[[nodiscard]] std::filesystem::path create_project_tmpdir(const std::filesystem::path &project_root)
{
    int max_tries = 1000;
    std::random_device random_device;
    std::mt19937 mersenne_prng(random_device());
    std::uniform_int_distribution<uint64_t> uniform_int_distribution(0);
    std::filesystem::path path;

    for (int i = 0; i <= max_tries; ++i) {
        std::stringstream string_stream;
        string_stream << std::hex << uniform_int_distribution(mersenne_prng);
        path = project_root / (".porytiles_tmp_" + string_stream.str());
        if (std::filesystem::create_directory(path)) {
            return path;
        }
        if (i == max_tries) {
            panic("create_project_tmpdir: exceeded maximum retries");
        }
    }
    panic("create_project_tmpdir: unreachable");
    return {}; // unreachable
}

ChainableResult<void>
save_layer_png(const PngRgbaImageSaver &saver, const Image<Rgba32> &layer_png, const std::filesystem::path &path)
{
    auto result = saver.save_to_file(layer_png, path);
    if (!result.has_value()) {
        return result;
    }
    return {};
}

ChainableResult<void> save_tiles_png(
    const PngIndexedImageSaver &saver,
    const Image<IndexPixel> &tiles_png,
    const std::filesystem::path &path,
    TilesPaletteMode tiles_palette_mode)
{
    auto result = saver.save_to_file(tiles_png, path, tiles_palette_mode);
    if (!result.has_value()) {
        return result;
    }
    return {};
}

ChainableResult<void> save_metatiles_bin(const std::vector<TilemapEntry> &entries, const std::filesystem::path &path)
{
    std::ofstream out{path};
    for (const auto &entry : entries) {
        // TODO: metatiles are a fixed format, these magic numbers could probably live somewhere else
        const auto tile_value = static_cast<uint16_t>(
            (entry.tile_index() & 0x3ffu) | ((entry.h_flip() & 1u) << 10u) | ((entry.v_flip() & 1u) << 11u) |
            ((entry.palette_index() & 0xfu) << 12u));
        out << static_cast<std::uint8_t>(tile_value);
        out << static_cast<std::uint8_t>(tile_value >> 8u);
    }
    out.flush();
    return {};
}

ChainableResult<void> save_palette(
    const Palette<Rgba32, palette::max_size> &palette, const std::filesystem::path &path, const FilePaletteSaver &saver)
{
    PT_TRY_CALL_CHAIN_ERR(saver.save(palette, path), void, "'{}': Failed to save.", FormatParam(path.c_str()));
    return {};
}

ChainableResult<void> save_porymap_anim_frame(
    const PngIndexedImageSaver &saver,
    const Image<IndexPixel> &frame,
    const std::filesystem::path &path,
    TilesPaletteMode tiles_palette_mode)
{
    auto result = saver.save_to_file(frame, path, tiles_palette_mode);
    if (!result.has_value()) {
        return result;
    }
    return {};
}

/// @brief Computes the staging path for an artifact and registers it for atomic commit.
///
/// @details
/// This function categorizes the artifact key and determines how it should be staged:
/// - For porytiles_src/porytiles_bin artifacts: Registers the directory in staged_directories
///   and returns a staging path that mirrors the relative structure under the directory.
/// - For special artifacts: Registers the file in staged_special_files and returns the staging path.
///
/// The staging path maintains the relative structure of files within their category directory,
/// allowing the entire directory to be atomically moved during commit.
///
/// @param transaction_root The root directory for the current transaction
/// @param project_root The project root directory
/// @param dest_key The artifact key (path relative to project root)
/// @param staged_directories Map to register staged directories
/// @param staged_special_files Vector to register special files
/// @return The path where the artifact should be written during the transaction
template <typename StagedDirectory>
ChainableResult<std::filesystem::path> compute_transaction_dest_path(
    const std::filesystem::path &transaction_root,
    const std::filesystem::path &project_root,
    const ArtifactKey &dest_key,
    std::map<std::filesystem::path, StagedDirectory> &staged_directories,
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> &staged_special_files)
{
    if (transaction_root.empty()) {
        return FormattableError{"No transaction in progress."};
    }

    const std::filesystem::path key_path{dest_key.key()};
    const auto path_info = categorize_artifact_key(key_path);

    if (path_info.category == ArtifactCategory::special) {
        // Special files are handled individually - stage them directly
        const auto staging_path = transaction_root / key_path;
        std::filesystem::create_directories(staging_path.parent_path());

        // Register for individual file commit
        const auto dest_path = project_root / key_path;
        staged_special_files.emplace_back(staging_path, dest_path);

        return staging_path;
    }

    // For porytiles_src/porytiles_bin, stage under a directory that will be atomically moved
    const auto &category_dir = path_info.directory;
    const auto dest_dir = project_root / category_dir;

    // Register this directory if not already registered
    if (!staged_directories.contains(dest_dir)) {
        // Create a unique staging directory for this category
        const auto staging_dir = transaction_root / category_dir;
        staged_directories[dest_dir] = StagedDirectory{staging_dir, dest_dir};
    }

    // Compute the path relative to the category directory
    const auto relative_within_category = std::filesystem::relative(key_path, category_dir);
    const auto staging_path = staged_directories[dest_dir].staging_path / relative_within_category;

    // Create parent directories in staging area
    std::filesystem::create_directories(staging_path.parent_path());

    return staging_path;
}

/// @brief Converts a vector of tiles into an image.
///
/// @details
/// Creates an image with tiles arranged in a grid. If width_tiles and height_tiles are both non-zero, uses those
/// dimensions. Otherwise, falls back to a single-row layout for backward compatibility.
///
/// @tparam PixelType The pixel type (Rgba32 or IndexPixel)
/// @param tiles The tiles to convert to an image
/// @param width_tiles Target width in tiles (0 = use single row)
/// @param height_tiles Target height in tiles (0 = use single row)
/// @pre If width_tiles and height_tiles are non-zero, width_tiles * height_tiles must equal tiles.size()
/// @return An image containing the tiles arranged in the specified grid
template <typename PixelType>
Image<PixelType> tiles_to_image(
    const std::vector<PixelTile<PixelType>> &tiles, std::size_t width_tiles = 0, std::size_t height_tiles = 0)
{
    if (tiles.empty()) {
        return Image<PixelType>{};
    }

    // Determine grid dimensions
    std::size_t tiles_per_row;
    std::size_t tiles_per_col;

    if (width_tiles > 0 && height_tiles > 0) {
        // Use specified dimensions
        if (width_tiles * height_tiles != tiles.size()) {
            panic(
                std::format(
                    "tiles_to_image: width_tiles ({}) * height_tiles ({}) != tiles.size() ({})",
                    width_tiles,
                    height_tiles,
                    tiles.size()));
        }
        tiles_per_row = width_tiles;
        tiles_per_col = height_tiles;
    }
    else {
        // Fall back to single row
        tiles_per_row = tiles.size();
        tiles_per_col = 1;
    }

    const std::size_t image_width = tiles_per_row * tile::side_length_pix;
    const std::size_t image_height = tiles_per_col * tile::side_length_pix;

    Image<PixelType> img{image_width, image_height};

    for (std::size_t tile_idx = 0; tile_idx < tiles.size(); ++tile_idx) {
        const auto &tile = tiles[tile_idx];
        const std::size_t tile_row = tile_idx / tiles_per_row;
        const std::size_t tile_col = tile_idx % tiles_per_row;
        const std::size_t pixel_row_offset = tile_row * tile::side_length_pix;
        const std::size_t pixel_col_offset = tile_col * tile::side_length_pix;

        for (std::size_t pixel_row = 0; pixel_row < tile::side_length_pix; ++pixel_row) {
            for (std::size_t pixel_col = 0; pixel_col < tile::side_length_pix; ++pixel_col) {
                const std::size_t dest_row = pixel_row_offset + pixel_row;
                const std::size_t dest_col = pixel_col_offset + pixel_col;
                img.set(dest_row, dest_col, tile.at(pixel_row, pixel_col));
            }
        }
    }

    return img;
}

/// @brief Template helper for writing animation frames to PNG files.
///
/// @details
/// This function unifies the logic for writing animation frames from both Porymap
/// (IndexPixel) and Porytiles (Rgba32) components. It handles both key frames and
/// regular numbered frames through the frame_index parameter.
///
/// @tparam PixelType The pixel type (Rgba32 or IndexPixel)
/// @tparam StagedDirectory The staged directory struct type
/// @tparam ComponentGetter Callable returning a const reference to the tileset component
/// @tparam SaveFunc Callable that saves the image to a file
/// @param dest_key The artifact key for the destination PNG file
/// @param src The source tileset
/// @param anim_name The name of the animation
/// @param frame_name The name of the frame
/// @param transaction_root The transaction root directory
/// @param project_root The project root directory
/// @param staged_directories Map to register staged directories
/// @param staged_special_files Vector to register special files
/// @param component_getter Lambda to get the appropriate component from tileset
/// @param save_func Lambda to save the image (handles indexed vs RGBA saving)
/// @param component_name Name of the component for error messages
/// @return ChainableResult<void> indicating success or failure
template <SupportsTransparency PixelType, typename StagedDirectory, typename ComponentGetter, typename SaveFunc>
ChainableResult<void> write_anim_frame_impl(
    const ArtifactKey &dest_key,
    const Tileset &src,
    const std::string &anim_name,
    const std::string &frame_name,
    const std::filesystem::path &transaction_root,
    const std::filesystem::path &project_root,
    std::map<std::filesystem::path, StagedDirectory> &staged_directories,
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> &staged_special_files,
    ComponentGetter component_getter,
    SaveFunc save_func,
    std::string_view component_name)
{
    const auto &component = component_getter(src);

    if (!component.has_anim(anim_name)) {
        return FormattableError{
            "animation '{}' not found in {} component",
            FormatParam{anim_name, Style::bold},
            FormatParam{std::string{component_name}}};
    }

    const auto &anim = component.anim_for_name(anim_name);

    // Get the appropriate frame
    const AnimFrame<PixelType> *frame_ptr = nullptr;
    if (frame_name != "key") {
        frame_ptr = &anim.frame_for_name(frame_name);
    }
    else {
        frame_ptr = &anim.key_frame();
    }

    // Convert tiles to image
    const auto &params = anim.params();
    auto img = tiles_to_image(frame_ptr->tiles(), params.width_tiles(), params.height_tiles());

    // Transfer palette from frame to image if present
    if (frame_ptr->has_palette()) {
        const auto &palette = frame_ptr->palette();
        std::vector<Rgba32> palette_vec;
        palette_vec.reserve(palette.size());
        for (std::size_t i = 0; i < palette.size(); ++i) {
            palette_vec.push_back(palette.at(i));
        }
        img.palette(std::move(palette_vec));
    }

    // Compute transaction path (keys are now relative to project_root)
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root, project_root, dest_key, staged_directories, staged_special_files),
        void,
        "Failed to compute transaction dest path.");

    // Save using provided save function
    return save_func(img, transaction_dest_path);
}

} // namespace

namespace porytiles {

ChainableResult<void> ProjectTilesetArtifactWriter::begin_transaction()
{
    if (!transaction_root_.empty()) {
        return FormattableError{"Transaction already in progress."};
    }

    // Create tmpdir inside project root to ensure same-filesystem for atomic moves
    transaction_root_ = create_project_tmpdir(project_root_);

    // Clear any stale tracking data
    staged_directories_.clear();
    staged_special_files_.clear();

    return {};
}

ChainableResult<void> ProjectTilesetArtifactWriter::commit()
{
    if (transaction_root_.empty()) {
        return FormattableError{"No transaction in progress."};
    }

    // If nothing was staged, just clean up
    if (staged_directories_.empty() && staged_special_files_.empty()) {
        std::filesystem::remove_all(transaction_root_);
        transaction_root_.clear();
        return {};
    }

    // Create backup root inside project for same-filesystem operations
    const auto backup_root = create_project_tmpdir(project_root_);

    // Track what we've moved for rollback
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> moved_directories; // (dest, backup)
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> backed_up_special_files;
    std::vector<std::filesystem::path> new_special_files;

    try {
        // Phase 1: Backup existing destination directories by moving them to backup
        for (const auto &[dest_dir, staged_info] : staged_directories_) {
            if (std::filesystem::exists(dest_dir)) {
                // Create backup path preserving structure
                const auto relative = std::filesystem::relative(dest_dir, project_root_);
                const auto backup_path = backup_root / relative;
                std::filesystem::create_directories(backup_path.parent_path());

                // Move existing directory to backup (atomic on same filesystem)
                std::filesystem::rename(dest_dir, backup_path);
                moved_directories.emplace_back(dest_dir, backup_path);
            }
        }

        // Phase 2: Atomic directory moves from staging to destination
        for (const auto &[dest_dir, staged_info] : staged_directories_) {
            // Create parent directories if needed
            std::filesystem::create_directories(dest_dir.parent_path());

            // Atomic move: rename staging directory to final destination
            std::filesystem::rename(staged_info.staging_path, dest_dir);
        }

        // Phase 3: Handle special files (like generated_anim_code.h)
        for (const auto &[staging_path, dest_path] : staged_special_files_) {
            // Backup existing special file if it exists
            if (std::filesystem::exists(dest_path)) {
                const auto relative = std::filesystem::relative(dest_path, project_root_);
                const auto backup_path = backup_root / relative;
                std::filesystem::create_directories(backup_path.parent_path());
                std::filesystem::copy_file(dest_path, backup_path);
                backed_up_special_files.emplace_back(dest_path, backup_path);
            }
            else {
                new_special_files.push_back(dest_path);
            }

            // Copy special file to destination (create dirs if needed)
            std::filesystem::create_directories(dest_path.parent_path());
            std::filesystem::copy_file(staging_path, dest_path, std::filesystem::copy_options::overwrite_existing);
        }

        // Phase 4: Success - clean up transaction and backup directories
        std::filesystem::remove_all(transaction_root_);
        std::filesystem::remove_all(backup_root);
        transaction_root_.clear();
        staged_directories_.clear();
        staged_special_files_.clear();

        return {};
    }
    catch (const std::filesystem::filesystem_error &e) {
        // Phase 5: Error occurred - rollback
        try {
            // Rollback moved directories: move backups back to their original locations
            for (const auto &[original_path, backup_path] : moved_directories) {
                if (std::filesystem::exists(backup_path)) {
                    // Remove any partially moved directory at destination
                    if (std::filesystem::exists(original_path)) {
                        std::filesystem::remove_all(original_path);
                    }
                    std::filesystem::rename(backup_path, original_path);
                }
            }

            // Rollback special files
            for (const auto &[original_path, backup_path] : backed_up_special_files) {
                if (std::filesystem::exists(backup_path)) {
                    std::filesystem::copy_file(
                        backup_path, original_path, std::filesystem::copy_options::overwrite_existing);
                }
            }

            // Remove new special files that were created
            for (const auto &new_file : new_special_files) {
                if (std::filesystem::exists(new_file)) {
                    std::filesystem::remove(new_file);
                }
            }
        }
        catch (const std::filesystem::filesystem_error &) {
            // Critical error during restore - best effort cleanup
        }

        // Clean up temporary directories
        if (std::filesystem::exists(backup_root)) {
            std::filesystem::remove_all(backup_root);
        }
        if (std::filesystem::exists(transaction_root_)) {
            std::filesystem::remove_all(transaction_root_);
        }
        transaction_root_.clear();
        staged_directories_.clear();
        staged_special_files_.clear();

        return FormattableError{"Failed to commit transaction: {}.", FormatParam{e.what()}};
    }
}

ChainableResult<void> ProjectTilesetArtifactWriter::rollback()
{
    if (transaction_root_.empty()) {
        return FormattableError{"No transaction in progress."};
    }

    try {
        if (std::filesystem::exists(transaction_root_)) {
            std::filesystem::remove_all(transaction_root_);
        }
        transaction_root_.clear();
        staged_directories_.clear();
        staged_special_files_.clear();
        return {};
    }
    catch (const std::filesystem::filesystem_error &e) {
        transaction_root_.clear();
        staged_directories_.clear();
        staged_special_files_.clear();
        return FormattableError{"Failed to rollback transaction: {}.", FormatParam{e.what()}};
    }
}

// Porymap artifacts
ChainableResult<void> ProjectTilesetArtifactWriter::write_metatiles_bin(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");
    return save_metatiles_bin(src.porymap_component().metatiles_bin(), transaction_dest_path);
}

ChainableResult<void>
ProjectTilesetArtifactWriter::write_metatile_attributes_bin(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");
    return save_metatile_attributes_bin(
        src.porymap_component().metatile_attributes_bin(), transaction_dest_path, *schema_);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_tiles_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_palette_mode_config,
        domain_config_->tiles_palette_mode(ConfigScopeType::tileset, src.name()),
        void,
        "Failed to get tiles_palette_mode config.");
    return save_tiles_png(
        *png_indexed_saver_,
        src.porymap_component().tiles_png(),
        transaction_dest_path,
        tiles_palette_mode_config.value());
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_porymap_palette_n(
    const ArtifactKey &dest_key, const Tileset &src, std::size_t index)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");
    const auto &palette = src.porymap_component().palette_at(index);
    if (palette.has_any_wildcards()) {
        panic("attempted to save a Porymap palette containing wildcards");
    }
    return save_palette(palette, transaction_dest_path, *palette_saver_);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_porymap_anim_frame(
    const ArtifactKey &dest_key, const Tileset &src, const std::string &anim_name, const std::string &frame_name)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_palette_mode_config,
        domain_config_->tiles_palette_mode(ConfigScopeType::tileset, src.name()),
        void,
        "Failed to get tiles_palette_mode config.");
    return write_anim_frame_impl<IndexPixel>(
        dest_key,
        src,
        anim_name,
        frame_name,
        transaction_root_,
        project_root_,
        staged_directories_,
        staged_special_files_,
        [](const Tileset &t) -> const auto & { return t.porymap_component(); },
        [this, &tiles_palette_mode_config](const Image<IndexPixel> &img, const std::filesystem::path &path) {
            return save_porymap_anim_frame(*png_indexed_saver_, img, path, tiles_palette_mode_config.value());
        },
        "Porymap");
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactWriter::write_porymap_anim_params(const ArtifactKey &dest_key, const Tileset &src)
{
    const auto &porymap_anims = src.porymap_component().anims();
    if (porymap_anims.empty()) {
        // If there are no anims, but the params file exists, remove it
        if (std::filesystem::exists(project_root_ / dest_key.key())) {
            std::filesystem::remove(project_root_ / dest_key.key());
        }
        return {};
    }

    std::map<DynamicCasedName, AnimParams> anim_params;
    for (const auto &[anim_name, anim] : porymap_anims) {
        anim_params[DynamicCasedName{anim_name}] = anim.params();
    }

    // Determine primary/secondary from metadata
    PT_TRY_ASSIGN_CHAIN_ERR(
        is_secondary,
        metadata_provider_.is_secondary(src.name()),
        void,
        "Failed to determine primary/secondary status for '{}'.",
        FormatParam(src.name(), Style::bold));
    const bool is_primary = !is_secondary;

    // Read tileset bin path from config based on primary/secondary status
    PT_TRY_ASSIGN_CHAIN_ERR(
        bin_path_base,
        is_primary ? infra_config_->tileset_paths_primary_bin(ConfigScopeType::tileset, src.name())
                   : infra_config_->tileset_paths_secondary_bin(ConfigScopeType::tileset, src.name()),
        void,
        "Failed to get tileset bin path config for '{}'.",
        FormatParam(src.name(), Style::bold));
    const std::filesystem::path tileset_path =
        std::filesystem::path{bin_path_base.value()} / extract_tileset_cased_name(src.name()).to_snake_case();

    PT_TRY_ASSIGN_CHAIN_ERR(
        generated_code,
        anim_code_generator_->generate(src.name(), tileset_path, anim_params, is_primary),
        void,
        "Failed to generate animation code for '{}'.",
        FormatParam(src.name(), Style::bold));

    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");

    std::ofstream out{transaction_dest_path};
    if (!out.is_open()) {
        return FormattableError{
            "Failed to open file for writing: '{}'.", FormatParam{transaction_dest_path.string(), Style::bold}};
    }
    out << generated_code;
    out.flush();

    return {};
}

// Porytiles artifacts
ChainableResult<void> ProjectTilesetArtifactWriter::write_bottom_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");
    return save_layer_png(*png_rgba_saver_, src.porytiles_component().bottom(), transaction_dest_path);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_middle_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");
    return save_layer_png(*png_rgba_saver_, src.porytiles_component().middle(), transaction_dest_path);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_top_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");
    return save_layer_png(*png_rgba_saver_, src.porytiles_component().top(), transaction_dest_path);
}

ChainableResult<void>
ProjectTilesetArtifactWriter::write_attributes_csv(const ArtifactKey &dest_key, const Tileset &src)
{
    const auto &attributes = src.porytiles_component().metatile_attributes();

    PT_TRY_ASSIGN_CHAIN_ERR(
        role_pins_cv,
        infra_config_->role_pins(ConfigScopeType::tileset, src.name()),
        void,
        "Failed to resolve role_pins.");
    const RolePinDefinitions &role_pins = role_pins_cv.value();

    // Role-pin validation runs here, mirroring the loader.
    PT_TRY_CALL_CHAIN_ERR(validate_role_pins_against_schema(role_pins, *schema_, *format_), void, "Invalid role pins.");

    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");

    std::ofstream out{transaction_dest_path};
    if (!out.is_open()) {
        return FormattableError{
            "Failed to open file for writing: '{}'.", FormatParam{transaction_dest_path.string(), Style::bold}};
    }

    // Write header: id plus every schema value field name in schema order, then one trailing pin column per role pin in
    // config order (header from effective_pin_column_name). A role-bearing field is not a true value column; the CSV
    // addresses a role's per-metatile value only through its pin column.
    std::string header = "id";
    for (const Field &field : schema_->value_fields()) {
        header += "," + field.name();
    }
    for (const RolePinDefinition &pin : role_pins) {
        header += "," + effective_pin_column_name(pin);
    }
    out << header << "\n";

    // A field absent from an attribute takes its schema default. MetatileAttribute::field() reads an absent field as
    // 0, but a schema field may declare a nonzero default, so the effective value must come through the schema. This
    // one value drives both cell rendering and all-default row omission.
    auto effective_value = [](const MetatileAttribute &attribute, const Field &field) -> std::uint32_t {
        return attribute.fields().contains(field.name()) ? attribute.field(field.name()) : field.default_value();
    };

    // Renders a row's field cells in schema order, without the id or layer_type. A provider-backed field renders its
    // value's constant name; a raw field renders the plain integer. has_provider() is the authority for that split,
    // and build_provider_map upholds has_provider() <=> map membership, so a missing provider is an internal bug.
    auto render_fields = [&](const MetatileAttribute &attribute,
                             std::size_t metatile_id) -> ChainableResult<std::string> {
        std::string cells{};
        for (const Field &field : schema_->value_fields()) {
            if (!cells.empty()) {
                cells += ",";
            }
            const std::uint32_t value = effective_value(attribute, field);
            if (!field.has_provider()) {
                cells += std::to_string(value);
                continue;
            }
            const auto provider_it = providers_->find(field.name());
            if (provider_it == providers_->end()) {
                panic(
                    std::format(
                        "write_attributes_csv: field '{}' has a provider spec but no provider was built for it",
                        field.name()));
            }
            PT_TRY_ASSIGN_CHAIN_ERR(
                field_name,
                provider_it->second->lookup(value),
                std::string,
                std::format("Failed to lookup {} name for metatile {}.", field.name(), metatile_id));
            cells += field_name;
        }
        return cells;
    };

    // A row is all-default only when every value field's effective value equals its schema default.
    auto is_all_default = [&](const MetatileAttribute &attribute) -> bool {
        for (const Field &field : schema_->value_fields()) {
            if (effective_value(attribute, field) != field.default_value()) {
                return false;
            }
        }
        return true;
    };

    // Omitting an all-default row is lossless: it reloads as an absent attribute, and downstream consumers (the
    // compiler in particular) materialize an absent attribute from the schema defaults, so the round trip reproduces
    // exactly the values the row was omitted for.
    auto omit_row = [&](const MetatileAttribute &attribute) -> bool { return is_all_default(attribute); };

    if (role_pins.empty()) {
        // No role pins: byte-identical to the historical output. Skip all-default rows, and if none survive write only
        // the header.
        std::size_t non_default_count = 0;
        for (const auto &attribute : attributes | std::views::values) {
            if (!omit_row(attribute)) {
                non_default_count++;
            }
        }

        if (non_default_count == 0) {
            out.flush();
            return {};
        }

        for (const auto &[metatile_id, attribute] : attributes) {
            if (omit_row(attribute)) {
                continue;
            }
            PT_TRY_ASSIGN_PASS_ERR(fields_str, render_fields(attribute, metatile_id), void);
            out << metatile_id << "," << fields_str << "\n";
        }

        out.flush();
        return {};
    }

    // Renders one pin cell for a role. Only an explicitly pinned value emits a token; an inferred/auto value (the
    // default, and everything a bin parser or decompiler produces) emits a blank cell. This keeps the "blank = auto"
    // workflow intact across a load/save round-trip. A row the user left blank carries no explicit value, so it must
    // not be written back as a pinned token that the next compile would then treat as an override.
    auto render_pin_cell = [](FieldRole role, const MetatileAttribute &attribute) -> std::string {
        switch (role) {
        case FieldRole::layer_type:
            return attribute.explicit_layer_type().has_value()
                       ? layer_type_csv_token(attribute.explicit_layer_type().value())
                       : std::string{};
        }
        panic("write_attributes_csv: unhandled FieldRole in role pin cell rendering");
    };

    // Renders the trailing pin cells for one attribute, one per role pin in config order, each preceded by a comma.
    auto render_pin_cells = [&](const MetatileAttribute &attribute) -> std::string {
        std::string cells{};
        for (const RolePinDefinition &pin : role_pins) {
            cells += "," + render_pin_cell(pin.role, attribute);
        }
        return cells;
    };

    // Role pins configured: emit one row per metatile so every metatile has a pin slot the user can fill. The attribute
    // map can be sparse (tileset creation stores only some ids), so materialize a default row for any missing id. Row
    // count comes from the Porytiles layer image dimensions, the same source LayerImageMetatileizer uses.
    const std::size_t layer_metatile_count = metatile::metatile_count(src.porytiles_component().bottom());
    MetatileAttribute default_attribute{}; // all-default fields, no explicit layer type (blank cell)
    for (const Field &field : schema_->value_fields()) {
        default_attribute.field(field.name(), field.default_value());
    }

    // A schema may carry zero value fields (only the role-bearing field, or nothing at all), in which case the id cell
    // is followed directly by the pin cells; a bare comma there would add a phantom empty column
    auto emit_row = [&](std::size_t metatile_id, const std::string &fields_str, const MetatileAttribute &attribute) {
        out << metatile_id;
        if (!fields_str.empty()) {
            out << "," << fields_str;
        }
        out << render_pin_cells(attribute) << "\n";
    };

    for (std::size_t metatile_id = 0; metatile_id < layer_metatile_count; ++metatile_id) {
        const auto it = attributes.find(metatile_id);
        const MetatileAttribute &attribute = it != attributes.end() ? it->second : default_attribute;

        PT_TRY_ASSIGN_PASS_ERR(fields_str, render_fields(attribute, metatile_id), void);
        emit_row(metatile_id, fields_str, attribute);
    }

    // Inconsistent input: emit any stored ids at or beyond the derived count so no stored attribute is silently
    // dropped.
    for (const auto &[metatile_id, attribute] : attributes) {
        if (metatile_id < layer_metatile_count) {
            continue;
        }
        PT_TRY_ASSIGN_PASS_ERR(fields_str, render_fields(attribute, metatile_id), void);
        emit_row(metatile_id, fields_str, attribute);
    }

    out.flush();
    return {};
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_porytiles_palette_n(
    const ArtifactKey &dest_key, const Tileset &src, std::size_t index)
{
    if (src.porytiles_component().palette_at(index).has_value()) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            transaction_dest_path,
            compute_transaction_dest_path(
                transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
            void,
            "Failed to compute transaction dest path.");

        return save_palette(
            src.porytiles_component().palette_at(index).value(), transaction_dest_path, *palette_saver_);
    }

    // No porytiles palette, do nothing
    return {};
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_porytiles_anim_frame(
    const ArtifactKey &dest_key, const Tileset &src, const std::string &anim_name, const std::string &frame_name)
{
    return write_anim_frame_impl<Rgba32>(
        dest_key,
        src,
        anim_name,
        frame_name,
        transaction_root_,
        project_root_,
        staged_directories_,
        staged_special_files_,
        [](const Tileset &t) -> const auto & { return t.porytiles_component(); },
        [this](const Image<Rgba32> &img, const std::filesystem::path &path) {
            return save_layer_png(*png_rgba_saver_, img, path);
        },
        "Porytiles");
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactWriter::write_porytiles_anim_params(const ArtifactKey &dest_key, const Tileset &src)
{
    const auto &porytiles_anims = src.porytiles_component().anims();
    const auto &primary_overrides = src.porytiles_component().primary_anim_overrides();

    if (porytiles_anims.empty() && primary_overrides.empty()) {
        // Unlike in write_porymap_anim_params, we don't need to delete anything here. That's because anim.json is
        // within porytiles_src dir, which is written using an atomic move. If the new porytiles_src dir doesn't contain
        // an anim.json, the old one will get wiped by the commit() call.
        return {};
    }

    // Extract params from animations
    std::map<DynamicCasedName, AnimParams> anim_params;
    for (const auto &[anim_name, anim] : porytiles_anims) {
        anim_params[DynamicCasedName{anim_name}] = anim.params();
    }

    // Convert primary_anim_overrides keys from std::string to DynamicCasedName
    std::map<DynamicCasedName, std::vector<AnimOverrideEntry>> primary_refs;
    for (const auto &[name, entries] : primary_overrides) {
        primary_refs[DynamicCasedName{name}] = entries;
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(
            transaction_root_, project_root_, dest_key, staged_directories_, staged_special_files_),
        void,
        "Failed to compute transaction dest path.");

    return anim_json_parser_->write(transaction_dest_path, anim_params, primary_refs);
}

} // namespace porytiles

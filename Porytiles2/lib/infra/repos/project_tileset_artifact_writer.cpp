#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <ranges>
#include <sstream>
#include <string>

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/utilities/filesystem_utils.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/result/error.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"

namespace {

using namespace porytiles2;

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
    TilesPalMode tiles_pal_mode)
{
    auto result = saver.save_to_file(tiles_png, path, tiles_pal_mode);
    if (!result.has_value()) {
        return result;
    }
    return {};
}

ChainableResult<void> save_metatiles_bin(const std::vector<TilemapEntry> &entries, const std::filesystem::path &path)
{
    std::ofstream out{path};
    for (const auto &entry : entries) {
        // TODO: does this code work as expected on a big-endian machine?
        const auto tile_value = static_cast<uint16_t>(
            (entry.tile_index() & 0x3ff) | ((entry.h_flip() & 1) << 10) | ((entry.v_flip() & 1) << 11) |
            ((entry.pal_index() & 0xf) << 12));
        out << static_cast<std::uint8_t>(tile_value);
        out << static_cast<std::uint8_t>(tile_value >> 8);
    }
    out.flush();
    return {};
}

ChainableResult<void>
save_metatile_attributes_bin(const std::vector<MetatileAttribute> &attributes, const std::filesystem::path &path)
{
    // TODO: will need different handling for firered attrs
    std::ofstream out{path};
    for (const auto &attribute : attributes) {
        const std::uint16_t behavior = attribute.behavior();
        const auto layer_type = static_cast<std::uint8_t>(attribute.layer_type());
        // TODO: does this code work as expected on a big-endian machine?
        const auto attribute_value = static_cast<std::uint16_t>((behavior & 0xff) | ((layer_type & 0xf) << 12));
        out << static_cast<std::uint8_t>(attribute_value);
        out << static_cast<std::uint8_t>(attribute_value >> 8);
    }
    out.flush();
    return {};
}

ChainableResult<void>
save_palette(const Palette<Rgba32, pal::max_size> &pal, const std::filesystem::path &path, const FilePalSaver &saver)
{
    const auto save_result = saver.save(pal, path);
    if (!save_result.has_value()) {
        return ChainableResult<void>{FormattableError{std::format("{}: failed to save", path.c_str())}, save_result};
    }
    return {};
}

ChainableResult<std::filesystem::path>
compute_transaction_dest_path(const std::filesystem::path &transaction_root, const ArtifactKey &dest_key)
{
    // TODO: just panic here
    if (transaction_root.empty()) {
        return FormattableError{"no transaction in progress"};
    }

    // Keys are now relative to project_root, so we can use them directly
    const auto transaction_dest_path = transaction_root / dest_key.key();

    std::filesystem::create_directories(transaction_dest_path.parent_path());

    return transaction_dest_path;
}

/**
 * @brief Converts a vector of tiles into an image.
 *
 * @details
 * Creates an image with tiles arranged in a grid. If width_tiles and height_tiles are both non-zero, uses those
 * dimensions. Otherwise, falls back to a single-row layout for backward compatibility.
 *
 * @tparam PixelType The pixel type (Rgba32 or IndexPixel)
 * @param tiles The tiles to convert to an image
 * @param width_tiles Target width in tiles (0 = use single row)
 * @param height_tiles Target height in tiles (0 = use single row)
 * @pre If width_tiles and height_tiles are non-zero, width_tiles * height_tiles must equal tiles.size()
 * @return An image containing the tiles arranged in the specified grid
 */
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

/**
 * @brief Template helper for writing animation frames to PNG files.
 *
 * @details
 * This function unifies the logic for writing animation frames from both Porymap
 * (IndexPixel) and Porytiles (Rgba32) components. It handles both key frames and
 * regular numbered frames through the frame_index parameter.
 *
 * @tparam PixelType The pixel type (Rgba32 or IndexPixel)
 * @tparam ComponentGetter Callable returning a const reference to the tileset component
 * @tparam SaveFunc Callable that saves the image to a file
 * @param dest_key The artifact key for the destination PNG file
 * @param src The source tileset
 * @param anim_name The name of the animation
 * @param frame_name The name of the frame
 * @param transaction_root The transaction root directory
 * @param project_root The project root directory
 * @param component_getter Lambda to get the appropriate component from tileset
 * @param save_func Lambda to save the image (handles indexed vs RGBA saving)
 * @param component_name Name of the component for error messages
 * @return ChainableResult<void> indicating success or failure
 */
template <SupportsTransparency PixelType, typename ComponentGetter, typename SaveFunc>
ChainableResult<void> write_anim_frame_impl(
    const ArtifactKey &dest_key,
    const Tileset &src,
    const std::string &anim_name,
    const std::string &frame_name,
    const std::filesystem::path &transaction_root,
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
    const AnimationFrame<PixelType> *frame_ptr = nullptr;
    // TODO: don't hardcode "key" here, this whole pattern is bad tbh
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
        const auto &pal = frame_ptr->palette();
        std::vector<Rgba32> pal_vec;
        pal_vec.reserve(pal.size());
        for (std::size_t i = 0; i < pal.size(); ++i) {
            pal_vec.push_back(pal.at(i));
        }
        img.palette(std::move(pal_vec));
    }

    // Compute transaction path (keys are now relative to project_root)
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root, dest_key),
        "failed to compute transaction dest path",
        void);

    // Save using provided save function
    return save_func(img, transaction_dest_path);
}

/**
 * @brief Template helper for writing config files.
 *
 * @details
 * This function unifies the logic for writing config and local_config files.
 *
 * @tparam ConfigGetter Callable that gets the config from the tileset
 * @param dest_key The artifact key for the destination config file
 * @param src The source tileset
 * @param transaction_root The transaction root directory
 * @param project_root The project root directory
 * @param config_getter Lambda to get the config lines from the component
 * @return ChainableResult<void> indicating success or failure
 */
template <typename ConfigGetter>
ChainableResult<void> write_config_impl(
    const ArtifactKey &dest_key,
    const Tileset &src,
    const std::filesystem::path &transaction_root,
    ConfigGetter config_getter)
{
    const auto &config = config_getter(src);

    if (config.empty()) {
        // No config to write
        return {};
    }

    // Keys are now relative to project_root
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root, dest_key),
        "failed to compute transaction dest path",
        void);

    std::ofstream out{transaction_dest_path};
    if (!out.is_open()) {
        return FormattableError{
            "{}: failed to open file for writing", FormatParam{transaction_dest_path.string(), Style::bold}};
    }

    for (const auto &line : config) {
        out << line << '\n';
    }
    out.flush();

    return {};
}

} // namespace

namespace porytiles2 {

ChainableResult<void> ProjectTilesetArtifactWriter::begin_transaction()
{
    if (!transaction_root_.empty()) {
        return FormattableError{"transaction already in progress"};
    }
    transaction_root_ = create_tmpdir();

    return {};
}

ChainableResult<void> ProjectTilesetArtifactWriter::commit()
{
    if (transaction_root_.empty()) {
        return FormattableError{"no transaction in progress"};
    }

    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> backed_up_files;
    std::vector<std::filesystem::path> new_files;

    const auto backup_root = create_tmpdir();
    try {
        // Phase 1: Collect all source files and their destinations
        std::vector<std::pair<std::filesystem::path, std::filesystem::path>> files_to_copy;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(transaction_root_)) {
            if (entry.is_regular_file()) {
                auto relative_path = std::filesystem::relative(entry.path(), transaction_root_);
                auto dest_path = project_root_ / relative_path;
                files_to_copy.emplace_back(entry.path(), dest_path);
            }
        }

        // Filter out files that are identical to existing files (skip unchanged artifacts)
        // TODO: once we have a logging system, print an info log here
        std::erase_if(files_to_copy, [](const auto &pair) { return files_are_identical(pair.first, pair.second); });

        // Phase 2: Backup existing files that will be overwritten
        for (const auto &dest : files_to_copy | std::views::values) {
            if (std::filesystem::exists(dest)) {
                auto backup_relative = std::filesystem::relative(dest, project_root_);
                auto backup_path = backup_root / backup_relative;

                // Create backup directory structure
                std::filesystem::create_directories(backup_path.parent_path());

                // Copy existing file to backup
                std::filesystem::copy_file(dest, backup_path);
                backed_up_files.emplace_back(dest, backup_path);
            }
        }

        // Phase 3: Copy all new files to their destinations
        for (const auto &[src, dest] : files_to_copy) {
            // Create parent directories if needed
            std::filesystem::create_directories(dest.parent_path());

            // Track whether this is a new file (not an overwrite)
            const bool is_new_file = !std::filesystem::exists(dest);

            // Copy the file (overwrite if exists)
            std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing);

            if (is_new_file) {
                new_files.push_back(dest);
            }
        }

        // Phase 4: Success - clean up transaction and backup directories
        std::filesystem::remove_all(transaction_root_);
        std::filesystem::remove_all(backup_root);
        transaction_root_.clear();

        return {};
    }
    catch (const std::filesystem::filesystem_error &e) {
        // Phase 5: Error occurred - restore backups and clean up new files
        try {
            // Restore backed up files
            for (const auto &[original_path, backup_path] : backed_up_files) {
                if (std::filesystem::exists(backup_path)) {
                    std::filesystem::copy_file(
                        backup_path, original_path, std::filesystem::copy_options::overwrite_existing);
                }
            }

            // Remove any new files that were created
            for (const auto &new_file : new_files) {
                if (std::filesystem::exists(new_file)) {
                    std::filesystem::remove(new_file);
                }
            }
        }
        catch (const std::filesystem::filesystem_error &restore_error) {
            // Critical error during restore - log but continue cleanup
            // TODO: emit a diagnostic here?
        }

        // Clean up temporary directories
        if (std::filesystem::exists(backup_root)) {
            std::filesystem::remove_all(backup_root);
        }
        if (std::filesystem::exists(transaction_root_)) {
            std::filesystem::remove_all(transaction_root_);
        }
        transaction_root_.clear();

        return FormattableError{"failed to commit transaction: {}", FormatParam{e.what()}};
    }
}

ChainableResult<void> ProjectTilesetArtifactWriter::rollback()
{
    if (transaction_root_.empty()) {
        return FormattableError{"no transaction in progress"};
    }

    try {
        if (std::filesystem::exists(transaction_root_)) {
            std::filesystem::remove_all(transaction_root_);
        }
        transaction_root_.clear();
        return {};
    }
    catch (const std::filesystem::filesystem_error &e) {
        transaction_root_.clear();
        return FormattableError{"failed to rollback transaction: {}", FormatParam{e.what()}};
    }
}

/*
 * Porymap artifacts
 */
ChainableResult<void> ProjectTilesetArtifactWriter::write_metatiles_bin(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);
    return save_metatiles_bin(src.porymap_component().metatiles_bin(), transaction_dest_path);
}

ChainableResult<void>
ProjectTilesetArtifactWriter::write_metatile_attributes_bin(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);
    return save_metatile_attributes_bin(src.porymap_component().metatile_attributes_bin(), transaction_dest_path);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_tiles_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);
    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_pal_mode_config,
        config_->tiles_pal_mode(ConfigScopeType::tileset, src.name()),
        "failed to get tiles_pal_mode config",
        void);
    return save_tiles_png(
        *png_indexed_saver_, src.porymap_component().tiles_png(), transaction_dest_path, tiles_pal_mode_config.value());
}

ChainableResult<void>
ProjectTilesetArtifactWriter::write_porymap_pal_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);
    return save_palette(src.porymap_component().pal_at(index), transaction_dest_path, *pal_saver_);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_porymap_anim_frame(
    const ArtifactKey &dest_key, const Tileset &src, const std::string &anim_name, const std::string &frame_name)
{
    return write_anim_frame_impl<IndexPixel>(
        dest_key,
        src,
        anim_name,
        frame_name,
        transaction_root_,
        [](const Tileset &t) -> const auto & { return t.porymap_component(); },
        [this](const Image<IndexPixel> &img, const std::filesystem::path &path) {
            return save_tiles_png(*png_indexed_saver_, img, path, TilesPalMode::true_color);
        },
        "Porymap");
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactWriter::write_porymap_anim_params(const ArtifactKey &dest_key, const Tileset &src)
{
    const auto &porymap_anims = src.porymap_component().anims();
    if (porymap_anims.empty()) {
        // No animations to write
        return {};
    }

    // Extract params from animations
    std::map<std::string, AnimationParams> anim_params;
    for (const auto &[anim_name, anim] : porymap_anims) {
        anim_params[anim_name] = anim.params();
    }

    /*
     * TODO: this is a hack. Technically, we should be passing in the frame paths here, so that the generator can write
     * the paths correctly. For now, we're just assuming that dest_key is pointing at the
     * "include/generated_anim_code.h" file, so two parent paths up is the tileset root folder.
     *
     * To fix this here, we need to figure out a way to pass the writer the frame paths. The best way to do this is to
     * refactor the artifact writer so that it writes animations in one shot. This refactor will be similar to the
     * refactor we did for the artifact reader, which allowed it to read animations in one shot.
     */
    // Keys are now relative to project_root, so no need to relativize
    const auto tileset_root_path = std::filesystem::path{dest_key.key()}.parent_path().parent_path();

    // TODO: determine if primary or secondary tileset from config
    const bool is_primary = true;

    auto code_result = anim_code_generator_->generate(src.name(), tileset_root_path, anim_params, is_primary);
    if (!code_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to generate animation code for '{}'", FormatParam{src.name(), Style::bold}},
            code_result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);

    // Write the generated code to file
    std::ofstream out{transaction_dest_path};
    if (!out.is_open()) {
        return FormattableError{
            "failed to open file for writing: {}", FormatParam{transaction_dest_path.string(), Style::bold}};
    }
    out << code_result.value();
    out.flush();

    return {};
}

/*
 * Porytiles artifacts
 */
ChainableResult<void> ProjectTilesetArtifactWriter::write_bottom_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);
    return save_layer_png(*png_rgba_saver_, src.porytiles_component().bottom(), transaction_dest_path);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_middle_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);
    return save_layer_png(*png_rgba_saver_, src.porytiles_component().middle(), transaction_dest_path);
}

ChainableResult<void> ProjectTilesetArtifactWriter::write_top_png(const ArtifactKey &dest_key, const Tileset &src)
{
    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);
    return save_layer_png(*png_rgba_saver_, src.porytiles_component().top(), transaction_dest_path);
}

ChainableResult<void>
ProjectTilesetArtifactWriter::write_attributes_csv(const ArtifactKey &dest_key, const Tileset &src)
{
    // TODO: implement
    return {};
}

ChainableResult<void>
ProjectTilesetArtifactWriter::write_porytiles_pal_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index)
{
    if (src.porytiles_component().pal_at(index).has_value()) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            transaction_dest_path,
            compute_transaction_dest_path(transaction_root_, dest_key),
            "failed to compute transaction dest path",
            void);

        return save_palette(src.porytiles_component().pal_at(index).value(), transaction_dest_path, *pal_saver_);
    }

    // No porytiles pal, do nothing
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
    if (porytiles_anims.empty()) {
        // No animations to write
        return {};
    }

    // Extract params from animations
    std::map<std::string, AnimationParams> anim_params;
    for (const auto &[anim_name, anim] : porytiles_anims) {
        anim_params[anim_name] = anim.params();
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        transaction_dest_path,
        compute_transaction_dest_path(transaction_root_, dest_key),
        "failed to compute transaction dest path",
        void);

    return anim_yaml_parser_->write(transaction_dest_path, anim_params);
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactWriter::write_config(const ArtifactKey &dest_key, const Tileset &src)
{
    return write_config_impl(dest_key, src, transaction_root_, [](const Tileset &t) -> const auto & {
        return t.porytiles_component().config();
    });
}

[[nodiscard]] ChainableResult<void>
ProjectTilesetArtifactWriter::write_local_config(const ArtifactKey &dest_key, const Tileset &src)
{
    return write_config_impl(dest_key, src, transaction_root_, [](const Tileset &t) -> const auto & {
        return t.porytiles_component().local_config();
    });
}

} // namespace porytiles2

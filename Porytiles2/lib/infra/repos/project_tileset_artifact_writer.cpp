#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <ranges>
#include <sstream>
#include <string>

#include "fmt/format.h"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace {

using namespace porytiles2;

std::filesystem::path create_tmpdir()
{
    int maxTries = 1000;
    auto tmpDir = std::filesystem::temp_directory_path();
    int i = 0;
    std::random_device randomDevice;
    std::mt19937 mersennePrng(randomDevice());
    std::uniform_int_distribution<uint64_t> uniformIntDistribution(0);
    std::filesystem::path path;
    while (true) {
        std::stringstream stringStream;
        stringStream << std::hex << uniformIntDistribution(mersennePrng);
        path = tmpDir / ("porytiles_" + stringStream.str());
        if (std::filesystem::create_directory(path)) {
            break;
        }
        if (i == maxTries) {
            panic("tmpfiles::createTmpdir getTmpdirPath took too many tries");
        }
        i++;
    }
    return path;
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
    /*
     * TODO: here, we need to check if dual-layer output is enabled. If so, then we can assume the compilation code
     * already set the correct LayerType attribute for each metatile. We can also assume that the compiler has validated
     * the tilemap entries to guarantee that at least one layer is completely transparent. So here, if dual layer is on,
     * we can simply filter out one of the groups of four entries if it is entirely transparent. It doesn't actually
     * matter which, as long as we filter on a multiple-of-four boundary. Since the attribute layer type is already set,
     * we just need to get rid of the first transparent layer we find. If the next layer after is also transparent, it
     * will still work correctly.
     */
    std::ofstream out{path};
    for (const auto &entry : entries) {
        // TODO: does this code work as expected on a big-endian machine?
        const auto tile_value = static_cast<uint16_t>(
            (entry.tile_index() & 0x3FF) | ((entry.hflip() & 1) << 10) | ((entry.vflip() & 1) << 11) |
            ((entry.pal_index() & 0xF) << 12));
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
        const auto attribute_value = static_cast<std::uint16_t>((behavior & 0xff) | ((layer_type & 0xf) << 12));
        out << static_cast<std::uint8_t>(attribute_value);
        out << static_cast<std::uint8_t>(attribute_value >> 8);
    }
    out.flush();
    return {};
}

ChainableResult<void> save_palette(const RgbaPal &pal, const std::filesystem::path &path, const FilePalSaver &saver)
{
    const auto save_result = saver.save(pal, path);
    if (!save_result.has_value()) {
        return ChainableResult<void>{FormattableError{fmt::format("{}: failed to save", path.c_str())}, save_result};
    }
    return {};
}

} // namespace

namespace porytiles2 {

Result<void> ProjectTilesetArtifactWriter::begin_transaction()
{
    if (!transaction_root_.empty()) {
        return std::unexpected{"transaction already in progress"};
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

Result<void> ProjectTilesetArtifactWriter::rollback()
{
    if (transaction_root_.empty()) {
        return std::unexpected{"no transaction in progress"};
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
        return std::unexpected{fmt::format("failed to rollback transaction: {}", e.what())};
    }
}

ChainableResult<void>
ProjectTilesetArtifactWriter::write(const ArtifactKey &dest_key, const TilesetArtifact &artifact, const Tileset &src)
{
    if (transaction_root_.empty()) {
        return FormattableError{"no transaction in progress"};
    }

    // Compute the destination path within the transaction directory
    const auto relative_path = std::filesystem::path{dest_key.key()}.lexically_relative(project_root_);
    const auto transaction_dest_path = transaction_root_ / relative_path;

    // Create parent directories if needed
    std::filesystem::create_directories(transaction_dest_path.parent_path());

    // Handle different artifact types
    switch (artifact.type()) {
    // Porytiles artifacts
    case TilesetArtifact::Type::bottom_png:
        return save_layer_png(*png_rgba_saver_, src.porytiles_component().bottom(), transaction_dest_path);
    case TilesetArtifact::Type::middle_png:
        return save_layer_png(*png_rgba_saver_, src.porytiles_component().middle(), transaction_dest_path);
    case TilesetArtifact::Type::top_png:
        return save_layer_png(*png_rgba_saver_, src.porytiles_component().top(), transaction_dest_path);
    case TilesetArtifact::Type::attributes_csv:
        // TODO: implement
        return {};
    case TilesetArtifact::Type::porytiles_anim_frame:
        // TODO: implement
        return {};
    case TilesetArtifact::Type::pal_override_n:
        // TODO: implement
        return {};

    // Porymap artifacts
    case TilesetArtifact::Type::metatiles_bin:
        return save_metatiles_bin(src.porymap_component().metatiles_bin(), transaction_dest_path);
    case TilesetArtifact::Type::metatile_attributes_bin:
        return save_metatile_attributes_bin(src.porymap_component().metatile_attributes(), transaction_dest_path);
    case TilesetArtifact::Type::tiles_png:
        return save_tiles_png(
            *png_indexed_saver_,
            src.porymap_component().tiles_png(),
            transaction_dest_path,
            config_->tiles_pal_mode(src.name()));
    case TilesetArtifact::Type::porymap_anim_frame:
        // TODO: implement
        return {};
    case TilesetArtifact::Type::pal_n: {
        if (!artifact.index().has_value()) {
            panic("took TilesetArtifact::Type::pal_n branch but missing pal index");
        }
        const auto index = artifact.index().value();
        return save_palette(src.porymap_component().pal_at(index), transaction_dest_path, *pal_saver_);
    }

    // Default case
    default:
        panic("unhandled TilesetArtifact::Type");
    }
}

} // namespace porytiles2

#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/infra/services/file_pal_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Parses a metatiles.bin file into TilemapEntry objects.
 *
 * @details
 * Reads a binary file containing 2-byte tilemap entries. Each entry encodes:
 * - Bits 0-9: tile index (10 bits)
 * - Bit 10: horizontal flip flag
 * - Bit 11: vertical flip flag
 * - Bits 12-15: palette index (4 bits)
 *
 * @param path Absolute path to the metatiles.bin file
 * @pre File must exist and be readable
 * @pre File size must be a multiple of 2 bytes
 * @return Vector of parsed TilemapEntry objects, or error if file is invalid/corrupted
 */
[[nodiscard]] ChainableResult<std::vector<TilemapEntry>> parse_metatiles_bin(const std::filesystem::path &path);

/**
 * @brief Parses a metatile_attributes.bin file for Emerald format.
 *
 * @details
 * Reads a binary file containing 2-byte metatile attribute entries. Each entry encodes:
 * - Bits 0-7: terrain/behavior value
 * - Bits 12-15: layer type
 *
 * @param path Absolute path to the metatile_attributes.bin file
 * @pre File must exist and be readable
 * @pre File size must be a multiple of 2 bytes (attr::bytes_per_attr_emerald)
 * @return Vector of parsed MetatileAttribute objects, or error if file is invalid/corrupted
 */
[[nodiscard]] ChainableResult<std::vector<MetatileAttribute>>
parse_emerald_metatile_attributes(const std::filesystem::path &path);

/**
 * @brief Loads an indexed PNG file (e.g., tiles.png).
 *
 * @details
 * Uses the provided loader to read an indexed-color PNG file and return the image data.
 *
 * @param path Absolute path to the PNG file
 * @param loader The PNG loader service to use
 * @pre File must exist and be a valid indexed PNG
 * @return The loaded indexed image, or error if loading fails
 */
[[nodiscard]] ChainableResult<std::unique_ptr<Image<IndexPixel>>>
load_indexed_png(const std::filesystem::path &path, const PngIndexedImageLoader &loader);

/**
 * @brief Loads a Porymap palette file (e.g., 00.pal).
 *
 * @details
 * Uses the provided loader to read a palette file in JASC or other supported format.
 *
 * @param path Absolute path to the palette file
 * @param loader The palette loader service to use
 * @pre File must exist and be a valid palette file
 * @return The loaded palette, or error if loading fails
 */
[[nodiscard]] ChainableResult<Palette<Rgba32, pal::max_size>>
load_porymap_palette(const std::filesystem::path &path, const FilePalLoader &loader);

} // namespace porytiles2

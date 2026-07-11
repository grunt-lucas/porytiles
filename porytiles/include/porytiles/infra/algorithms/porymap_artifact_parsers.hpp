#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/infra/services/file_pal_loader.hpp"
#include "porytiles/infra/services/png_indexed_image_loader.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Parses a metatiles.bin file into TilemapEntry objects.
///
/// @details
/// Reads a binary file containing 2-byte tilemap entries. Each entry encodes:
/// - Bits 0-9: tile index (10 bits)
/// - Bit 10: horizontal flip flag
/// - Bit 11: vertical flip flag
/// - Bits 12-15: palette index (4 bits)
///
/// @param path Absolute path to the metatiles.bin file
/// @pre File must exist and be readable
/// @pre File size must be a multiple of 2 bytes
/// @return Vector of parsed TilemapEntry objects, or error if file is invalid/corrupted
[[nodiscard]] ChainableResult<std::vector<TilemapEntry>> parse_metatiles_bin(const std::filesystem::path &path);

/// @brief Parses a metatile_attributes.bin file according to a metatile attribute schema.
///
/// @details
/// Reads a binary file containing schema.attr_bytes() bytes per attribute entry, little-endian. Each
/// schema field's value is extracted through its mask and offset, and the structural layer type is
/// extracted through the schema's layer_type_mask(). The schema is the sole authority on the layout;
/// this function carries no hardcoded masks.
///
/// @param path Absolute path to the metatile_attributes.bin file
/// @param schema The attribute schema describing the binary layout
/// @pre File must exist and be readable
/// @pre File size must be a multiple of schema.attr_bytes()
/// @return Vector of parsed MetatileAttribute objects, or error if file is invalid/corrupted
[[nodiscard]] ChainableResult<std::vector<MetatileAttribute>>
parse_metatile_attributes(const std::filesystem::path &path, const Schema &schema);

/// @brief Writes metatile attributes to a metatile_attributes.bin file according to a schema.
///
/// @details
/// The inverse of parse_metatile_attributes: each attribute's schema fields and structural layer type
/// are packed into a single word through the schema's masks and offsets, then written as
/// schema.attr_bytes() bytes, little-endian.
///
/// @param attributes The attributes to write, in metatile order
/// @param path Absolute path of the metatile_attributes.bin file to write
/// @param schema The attribute schema describing the binary layout
/// @return Success, or an error if the file cannot be written
[[nodiscard]] ChainableResult<void> save_metatile_attributes_bin(
    const std::vector<MetatileAttribute> &attributes, const std::filesystem::path &path, const Schema &schema);

/// @brief Loads an indexed PNG file (e.g., tiles.png).
///
/// @details
/// Uses the provided loader to read an indexed-color PNG file and return the image data.
///
/// @param path Absolute path to the PNG file
/// @param loader The PNG loader service to use
/// @pre File must exist and be a valid indexed PNG
/// @return The loaded indexed image, or error if loading fails
[[nodiscard]] ChainableResult<std::unique_ptr<Image<IndexPixel>>>
load_indexed_png(const std::filesystem::path &path, const PngIndexedImageLoader &loader);

/// @brief Loads a Porymap palette file (e.g., 00.pal).
///
/// @details
/// Uses the provided loader to read a palette file in JASC or other supported format.
///
/// @param path Absolute path to the palette file
/// @param loader The palette loader service to use
/// @pre File must exist and be a valid palette file
/// @return The loaded palette, or error if loading fails
[[nodiscard]] ChainableResult<Palette<Rgba32, pal::max_size>>
load_porymap_palette(const std::filesystem::path &path, const FilePalLoader &loader);

} // namespace porytiles

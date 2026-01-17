#pragma once

/**
 * @file animation_frame_loader.hpp
 *
 * @brief Shared helper for loading animation frames from PNG files.
 *
 * @details
 * This header provides `load_animation_frame_from_png<>()`, a template function that encapsulates the common logic for
 * loading an animation frame PNG file and extracting tiles from it. This is used by both:
 * - ProjectVanillaAnimImporter (imports vanilla tileset animations as IndexPixel)
 * - ProjectTilesetArtifactReader (loads Porytiles/Porymap animation frames as Rgba32 or IndexPixel)
 *
 * The helper handles:
 * 1. Loading the PNG file via the provided loader
 * 2. Calculating width_tiles/height_tiles from image dimensions
 * 3. Extracting tiles from the image via extract_tiles_from_image()
 * 4. Creating an AnimationFrame with the extracted tiles
 * 5. Setting the palette on the frame if the image has one
 *
 * Each caller is responsible for:
 * - Adding the frame to their Animation/component data structure
 * - Updating AnimationParams with dimensions (typically from first frame)
 */

#include <filesystem>
#include <string>

#include "porytiles2/domain/algorithms/tile_extractors.hpp"
#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/supports_transparency.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Result of loading an animation frame from a PNG file.
 *
 * @details
 * Contains the loaded AnimationFrame along with the frame dimensions in tiles. The dimensions are calculated from the
 * source image and should be used to update AnimationParams (typically only from the first frame, since all frames in
 * an animation must have the same dimensions).
 *
 * @tparam PixelType The pixel type for the frame tiles (Rgba32 or IndexPixel)
 */
template <SupportsTransparency PixelType>
struct FrameLoadResult {
    AnimationFrame<PixelType> frame;
    std::size_t width_tiles;
    std::size_t height_tiles;
};

/**
 * @brief Loads an animation frame from a PNG file.
 *
 * @details
 * This template function provides the shared logic for loading animation frame PNG files. It handles the common
 * workflow of:
 * 1. Loading the PNG via the provided loader
 * 2. Calculating frame dimensions in tiles (width_tiles, height_tiles)
 * 3. Extracting tiles from the image in row-major order
 * 4. Creating an AnimationFrame with the specified name and extracted tiles
 * 5. Copying the image's palette to the frame if present
 *
 * The function is parameterized by pixel type and loader type, allowing it to work with both:
 * - PngIndexedImageLoader + IndexPixel (for Porymap component / vanilla imports)
 * - PngRgbaImageLoader + Rgba32 (for Porytiles component)
 *
 * @tparam PixelType The pixel type for tiles; must satisfy SupportsTransparency concept
 * @tparam LoaderType The PNG loader type; must have a load_from_file(path) method returning ChainableResult
 * @param png_path The absolute path to the PNG file to load
 * @param frame_name The name to assign to the frame (e.g., "0", "1", "key")
 * @param loader The PNG image loader instance to use
 * @pre PNG file must exist at png_path
 * @pre Image dimensions must be multiples of 8 (tile size)
 * @return FrameLoadResult containing the AnimationFrame and its dimensions, or error if loading fails
 */
template <SupportsTransparency PixelType, typename LoaderType>
[[nodiscard]] ChainableResult<FrameLoadResult<PixelType>> load_animation_frame_from_png(
    const std::filesystem::path &png_path, const std::string &frame_name, const LoaderType &loader)
{
    // Step 1: Load the PNG file
    auto image_result = loader.load_from_file(png_path);
    if (!image_result.has_value()) {
        return ChainableResult<FrameLoadResult<PixelType>>{
            FormattableError{"{}: failed to load animation frame PNG", FormatParam{png_path.string(), Style::bold}},
            image_result};
    }

    const Image<PixelType> &img = *image_result.value();

    // Step 2: Calculate dimensions in tiles
    const std::size_t width_tiles = img.width() / tile::side_length_pix;
    const std::size_t height_tiles = img.height() / tile::side_length_pix;

    // Step 3: Extract tiles from the image
    std::vector<PixelTile<PixelType>> tiles = extract_tiles_from_image(img);

    // Step 4: Create the AnimationFrame
    AnimationFrame<PixelType> frame{frame_name, std::move(tiles)};

    // Step 5: Set palette if the image has one
    if (img.palette().has_value()) {
        frame.palette(Palette<Rgba32>{img.palette().value()});
    }

    return FrameLoadResult<PixelType>{std::move(frame), width_tiles, height_tiles};
}

} // namespace porytiles2

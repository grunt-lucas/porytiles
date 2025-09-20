#include "porytiles2/domain/compile_tileset/operations/construct_rgba_metatiles_op.hpp"

#include <typeindex>

#include "fmt/format.h"

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/model/rgba_metatile.hpp"
#include "porytiles2/domain/model/tile.hpp"
#include "porytiles2/domain/orchestration/operand_bundle.hpp"
#include "porytiles2/templates/error.hpp"

namespace porytiles2 {

std::vector<OperandDeclaration> ConstructRgbaMetatilesOp::declare_inputs() const
{
    return {
        OperandDeclaration{"bottom.png", std::type_index{typeid(Image<Rgba32>)}},
        OperandDeclaration{"middle.png", std::type_index{typeid(Image<Rgba32>)}},
        OperandDeclaration{"top.png", std::type_index{typeid(Image<Rgba32>)}}};
}

std::vector<OperandDeclaration> ConstructRgbaMetatilesOp::declare_outputs() const
{
    return {OperandDeclaration{"rgba_metatiles", std::type_index{typeid(std::vector<RgbaMetatile>)}}};
}

ChainableResult<OperandBundle> ConstructRgbaMetatilesOp::execute(const OperandBundle &inputs)
{
    const auto bottom_opt = inputs.get_unwrapped<Image<Rgba32>>("bottom.png");
    const auto middle_opt = inputs.get_unwrapped<Image<Rgba32>>("middle.png");
    const auto top_opt = inputs.get_unwrapped<Image<Rgba32>>("top.png");

    if (!bottom_opt || !middle_opt || !top_opt) {
        return BasicError{"Failed to get one or more layer images from inputs"};
    }

    const auto &bottom = *bottom_opt;
    const auto &middle = *middle_opt;
    const auto &top = *top_opt;

    // Validate that all images have the same dimensions
    if (bottom.width() != middle.width() || bottom.height() != middle.height() || bottom.width() != top.width() ||
        bottom.height() != top.height()) {
        return BasicError{fmt::format(
            "Layer images have mismatched dimensions: bottom={}x{}, middle={}x{}, top={}x{}",
            bottom.width(),
            bottom.height(),
            middle.width(),
            middle.height(),
            top.width(),
            top.height())};
    }

    // Validate that dimensions are multiples of metatile size
    constexpr std::size_t metatile_size = RgbaMetatile::metatile_side_length;
    if (bottom.width() % metatile_size != 0 || bottom.height() % metatile_size != 0) {
        return BasicError{fmt::format(
            "Image dimensions must be multiples of {}, got {}x{}", metatile_size, bottom.width(), bottom.height())};
    }

    const std::size_t metatiles_per_row = bottom.width() / metatile_size;
    const std::size_t metatiles_per_col = bottom.height() / metatile_size;
    const std::size_t total_metatiles = metatiles_per_row * metatiles_per_col;

    std::vector<RgbaMetatile> rgba_metatiles;
    rgba_metatiles.reserve(total_metatiles);

    // Process each 16x16 metatile region
    for (std::size_t metatile_row = 0; metatile_row < metatiles_per_row; ++metatile_row) {
        for (std::size_t metatile_col = 0; metatile_col < metatiles_per_col; ++metatile_col) {
            RgbaMetatile metatile;

            // Each metatile has tiles_per_side x tiles_per_side tiles, each tile is tile_side_length x tile_side_length
            // pixels Tile indices in a metatile are arranged as: 0 1 2 3
            constexpr std::size_t tiles_per_side = RgbaMetatile::tiles_per_side;
            constexpr std::size_t tiles_per_metatile = RgbaMetatile::tiles_per_metatile;
            constexpr std::size_t tile_side_length = Tile<Rgba32>::tile_side_length;

            for (std::size_t tile_idx = 0; tile_idx < tiles_per_metatile; ++tile_idx) {
                // Calculate tile position within the metatile
                const std::size_t tile_row = tile_idx / tiles_per_side;
                const std::size_t tile_col = tile_idx % tiles_per_side;

                // Calculate pixel offsets for this tile
                const std::size_t pixel_row_offset = metatile_row * metatile_size + tile_row * tile_side_length;
                const std::size_t pixel_col_offset = metatile_col * metatile_size + tile_col * tile_side_length;

                // Create tiles for each layer
                Tile<Rgba32> bottom_tile;
                Tile<Rgba32> middle_tile;
                Tile<Rgba32> top_tile;

                // Copy pixels from source images to tiles
                for (std::size_t pixel_row = 0; pixel_row < tile_side_length; ++pixel_row) {
                    for (std::size_t pixel_col = 0; pixel_col < tile_side_length; ++pixel_col) {
                        const std::size_t src_row = pixel_row_offset + pixel_row;
                        const std::size_t src_col = pixel_col_offset + pixel_col;

                        bottom_tile.set(pixel_row, pixel_col, bottom.at(src_row, src_col));
                        middle_tile.set(pixel_row, pixel_col, middle.at(src_row, src_col));
                        top_tile.set(pixel_row, pixel_col, top.at(src_row, src_col));
                    }
                }

                // Set tiles in the metatile
                metatile.set_bottom(tile_idx, bottom_tile);
                metatile.set_middle(tile_idx, middle_tile);
                metatile.set_top(tile_idx, top_tile);
            }

            rgba_metatiles.push_back(std::move(metatile));
        }
    }

    OperandBundle outputs;
    outputs.put("rgba_metatiles", std::move(rgba_metatiles));

    return outputs;
}

} // namespace porytiles2

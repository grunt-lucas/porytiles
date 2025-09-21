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
        return BasicError{"failed to get one or more layer images from inputs"};
    }

    const auto &bottom = *bottom_opt;
    const auto &middle = *middle_opt;
    const auto &top = *top_opt;

    // Validate that all images have the same dimensions
    if (bottom.width() != middle.width() || bottom.height() != middle.height() || bottom.width() != top.width() ||
        bottom.height() != top.height()) {
        return BasicError{fmt::format(
            "layer images have mismatched dimensions: bottom={}x{}, middle={}x{}, top={}x{}",
            bottom.width(),
            bottom.height(),
            middle.width(),
            middle.height(),
            top.width(),
            top.height())};
    }

    // Tileize each layer image
    const auto bottom_tiles_result = tileizer_.tileize(bottom);
    if (!bottom_tiles_result.has_value()) {
        return ChainableResult<OperandBundle>::chain_together(
            BasicError{"failed to tileize bottom layer"}, bottom_tiles_result);
    }
    const auto &bottom_tiles = bottom_tiles_result.value();

    const auto middle_tiles_result = tileizer_.tileize(middle);
    if (!middle_tiles_result.has_value()) {
        return ChainableResult<OperandBundle>::chain_together(
            BasicError{"failed to tileize middle layer"}, middle_tiles_result);
    }
    const auto &middle_tiles = middle_tiles_result.value();

    const auto top_tiles_result = tileizer_.tileize(top);
    if (!top_tiles_result.has_value()) {
        return ChainableResult<OperandBundle>::chain_together(
            BasicError{"failed to tileize top layer"}, top_tiles_result);
    }
    const auto &top_tiles = top_tiles_result.value();

    /*
     * Validate that dimensions are multiples of metatile size. We already validated that all image dimensions are
     * identical, so we can just check bottom here as a surrogate for the other two layers. Additionally, we already
     * checked in the tileization step if the image dimensions were a multiple of 8. Now, we check that the image
     * dimensions are a multiple of 16 to confirm that it can be correctly metatileized.
     */
    if (bottom.width() % RgbaMetatile::metatile_side_length != 0 ||
        bottom.height() % RgbaMetatile::metatile_side_length != 0) {
        return BasicError{fmt::format(
            "image dimensions must be multiples of {}, got {}x{}",
            RgbaMetatile::metatile_side_length,
            bottom.width(),
            bottom.height())};
    }

    const std::size_t metatiles_per_row = bottom.width() / RgbaMetatile::metatile_side_length;
    const std::size_t metatiles_per_col = bottom.height() / RgbaMetatile::metatile_side_length;
    const std::size_t total_metatiles = metatiles_per_row * metatiles_per_col;

    std::vector<RgbaMetatile> rgba_metatiles;
    rgba_metatiles.reserve(total_metatiles);

    // Process each 16x16 metatile region
    for (std::size_t metatile_row = 0; metatile_row < metatiles_per_row; ++metatile_row) {
        for (std::size_t metatile_col = 0; metatile_col < metatiles_per_col; ++metatile_col) {
            RgbaMetatile metatile;

            for (std::size_t tile_idx = 0; tile_idx < RgbaMetatile::tiles_per_metatile; ++tile_idx) {
                // Calculate tile position within the metatile
                const std::size_t tile_row = tile_idx / RgbaMetatile::tiles_per_side;
                const std::size_t tile_col = tile_idx % RgbaMetatile::tiles_per_side;

                // Calculate which tile index we need from the tileized arrays
                const std::size_t tiles_per_image_row = bottom.width() / RgbaTile::tile_side_length;
                const std::size_t global_tile_row = metatile_row * RgbaMetatile::tiles_per_side + tile_row;
                const std::size_t global_tile_col = metatile_col * RgbaMetatile::tiles_per_side + tile_col;
                const std::size_t global_tile_idx = global_tile_row * tiles_per_image_row + global_tile_col;

                // Set tiles in the metatile from the tileized arrays
                metatile.set_bottom(tile_idx, bottom_tiles[global_tile_idx]);
                metatile.set_middle(tile_idx, middle_tiles[global_tile_idx]);
                metatile.set_top(tile_idx, top_tiles[global_tile_idx]);
            }

            rgba_metatiles.push_back(std::move(metatile));
        }
    }

    OperandBundle outputs;
    outputs.put("rgba_metatiles", std::move(rgba_metatiles));

    return outputs;
}

} // namespace porytiles2

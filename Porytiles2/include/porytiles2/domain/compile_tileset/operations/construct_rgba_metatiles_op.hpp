#pragma once

#include <vector>

#include "porytiles2/domain/orchestration/operand_declaration.hpp"
#include "porytiles2/domain/orchestration/operation.hpp"
#include "porytiles2/domain/services/rgba_image_tileizer.hpp"

namespace porytiles2 {

/**
 * @brief Constructs RGBA metatiles from bottom, middle, and top layer images.
 *
 * @details
 * This operation takes three RGBA layer images (bottom, middle, top) as input and produces a vector of RgbaMetatile
 * objects. It serves as the second step in the tileset compilation pipeline, processing the layer images supplied by
 * the TilesetSupplierOp.
 */
class ConstructRgbaMetatilesOp final : public Operation {
  public:
    ConstructRgbaMetatilesOp() = default;

    [[nodiscard]] std::vector<OperandDeclaration> declare_inputs() const override;
    [[nodiscard]] std::vector<OperandDeclaration> declare_outputs() const override;

  protected:
    [[nodiscard]] ChainableResult<OperandBundle> execute(const OperandBundle &inputs) override;

  private:
    RgbaImageTileizer tileizer_;
};

} // namespace porytiles2

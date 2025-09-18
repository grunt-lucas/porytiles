#pragma once

#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/model/porytiles_tileset_component.hpp"
#include "porytiles2/domain/orchestration/operand_declaration.hpp"
#include "porytiles2/domain/orchestration/operation.hpp"

namespace porytiles2 {

/**
 * @brief Supplies bottom, middle, and top RGBA layer images from a PorytilesTilesetComponent.
 *
 * @details
 * This operation takes no inputs and outputs three RGBA layer images (bottom, middle, top)
 * from a PorytilesTilesetComponent provided during construction. It serves as the initial
 * step in the tileset compilation pipeline.
 */
class TilesetSupplierOp final : public Operation {
  public:
    /**
     * @brief Constructs a TilesetSupplierOp with a pointer to the tileset component.
     *
     * @param tileset_component Pointer to the PorytilesTilesetComponent from which to supply images
     */
    explicit TilesetSupplierOp(gsl::not_null<const PorytilesTilesetComponent *> tileset_component);

    [[nodiscard]] std::vector<OperandDeclaration> declare_inputs() const override;
    [[nodiscard]] std::vector<OperandDeclaration> declare_outputs() const override;

  protected:
    [[nodiscard]] Result<OperandBundle> execute(const OperandBundle &inputs) override;

  private:
    const PorytilesTilesetComponent *tileset_component_;
};

} // namespace porytiles2

#include "porytiles2/domain/compile_tileset/operations/tileset_supplier_op.hpp"

#include <typeindex>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/domain/orchestration/operand_bundle.hpp"

namespace porytiles2 {

TilesetSupplierOp::TilesetSupplierOp(gsl::not_null<const PorytilesTilesetComponent *> tileset_component)
    : tileset_component_{tileset_component}
{
}

std::vector<OperandDeclaration> TilesetSupplierOp::declare_inputs() const
{
    return {};
}

std::vector<OperandDeclaration> TilesetSupplierOp::declare_outputs() const
{
    return {
        OperandDeclaration{"bottom.png", std::type_index{typeid(Image<Rgba32>)}},
        OperandDeclaration{"middle.png", std::type_index{typeid(Image<Rgba32>)}},
        OperandDeclaration{"top.png", std::type_index{typeid(Image<Rgba32>)}}};
}

Result<OperandBundle> TilesetSupplierOp::execute(const OperandBundle &inputs)
{
    OperandBundle outputs;

    outputs.put("bottom.png", tileset_component_->bottom());
    outputs.put("middle.png", tileset_component_->middle());
    outputs.put("top.png", tileset_component_->top());

    return outputs;
}

} // namespace porytiles2

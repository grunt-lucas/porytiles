#pragma once

#include <memory>

#include "porytiles2/domain/model/porymap_layout_component.hpp"
#include "porytiles2/domain/model/porytiles_layout_component.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Service interface for compiling a PorytilesLayoutComponent into a PorymapLayoutComponent.
 *
 * @details
 * Service interface for compiling a PorytilesLayoutComponent into a PorymapLayoutComponent.
 */
class LayoutCompiler {
  public:
    virtual ~LayoutCompiler() = default;

    virtual Result<std::unique_ptr<PorymapLayoutComponent>> compile(const PorytilesLayoutComponent &layout) = 0;
};

} // namespace porytiles2

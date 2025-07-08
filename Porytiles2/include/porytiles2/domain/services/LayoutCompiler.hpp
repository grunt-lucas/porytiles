#pragma once

#include <memory>

#include "porytiles2/domain/model/aggregates/components/PorymapLayoutComponent.hpp"
#include "porytiles2/domain/model/aggregates/components/PorytilesLayoutComponent.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Service interface for compiling a PorytilesLayoutComponent into a PorymapLayoutComponent.
 *
 * @details
 * Service interface for compiling a PorytilesLayoutComponent into a PorymapLayoutComponent.
 */
class LayoutCompiler {
public:
  virtual ~LayoutCompiler() = default;

  virtual Result<std::unique_ptr<PorymapLayoutComponent>>
  Compile(const PorytilesLayoutComponent &layout) = 0;
};

} // namespace porytiles

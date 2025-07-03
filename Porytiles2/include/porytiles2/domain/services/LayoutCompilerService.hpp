#pragma once

#include <memory>

#include "porytiles2/domain/model/aggregates/components/PorymapLayoutComponent.hpp"
#include "porytiles2/domain/model/aggregates/components/PorytilesLayoutComponent.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief A domain service that provides functionality to compile a
 * PorytilesLayoutComponent to a PorymapLayoutComponent.
 */
class LayoutCompilerService {
public:
  virtual ~LayoutCompilerService() = default;

  virtual Result<std::unique_ptr<PorymapLayoutComponent>>
  Compile(const PorytilesLayoutComponent &layout) = 0;
};

} // namespace porytiles

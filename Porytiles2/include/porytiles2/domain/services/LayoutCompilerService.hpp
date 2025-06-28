#pragma once

#include <memory>

#include "porytiles2/domain/aggregates/PorymapLayout.hpp"
#include "porytiles2/domain/aggregates/PorytilesLayout.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief A domain service that provides functionality to compile PorytilesLayout to PorymapLayout.
 */
class LayoutCompilerService {
  public:
    virtual ~LayoutCompilerService() = default;

    /**
     * @brief Compiles a PorytilesLayout to a PorymapLayout.
     *
     * @param layout The PorytilesLayout to compile.
     * @return A PorymapLayout Result on success, otherwise an error description.
     */
    virtual Result<std::unique_ptr<PorymapLayout>> Compile(const PorytilesLayout &layout) = 0;
};

} // namespace porytiles

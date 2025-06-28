#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorymapLayout.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

class PorymapLayoutRepo {
public:
  virtual ~PorymapLayoutRepo() = default;

  /**
   * @brief Persists a new or existing PorymapLayout.
   *
   * @param layout The PorymapLayout aggregate to save.
   * @return An empty Result on success, otherwise an error description.
   */
  virtual Result<void> Save(const PorymapLayout &layout) = 0;

  /**
   * @brief Loads an existing PorymapLayout from storage.
   *
   * @param name The name of the PorymapLayout aggregate to load.
   * @return A PorymapLayout Result on success, otherwise an error description.
   */
  virtual Result<std::unique_ptr<PorymapLayout>>
  Load(const std::string &name) = 0;
};

} // namespace porytiles

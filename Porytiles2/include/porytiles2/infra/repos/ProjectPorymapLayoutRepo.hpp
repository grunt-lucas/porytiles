#pragma once

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorymapLayout.hpp"
#include "porytiles2/domain/repos/PorymapLayoutRepo.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Implementation of PorymapLayoutRepo that uses an in-filesystem
 * `pokeemerald` project as the backing store.
 */
class ProjectPorymapLayoutRepo final : public PorymapLayoutRepo {
public:
  Result<void> Save(const PorymapLayout &tileset) override;

  Result<std::unique_ptr<PorymapLayout>> Load(const std::string &name) override;
};

} // namespace porytiles

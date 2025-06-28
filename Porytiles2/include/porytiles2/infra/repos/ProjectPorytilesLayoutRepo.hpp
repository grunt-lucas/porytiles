#pragma once

#include <expected>
#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorytilesLayout.hpp"
#include "porytiles2/domain/repos/PorytilesLayoutRepo.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Implementation of PorytilesLayoutRepo that uses an in-filesystem
 * `pokeemerald` project as the backing store.
 */
class ProjectPorytilesLayoutRepo final : public PorytilesLayoutRepo {
public:
  Result<void> Save(const PorytilesLayout &layout) override;

  Result<std::unique_ptr<PorytilesLayout>>
  Load(const std::string &name) override;
};

} // namespace porytiles

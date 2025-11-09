#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Use case for verifying a primary tileset.
 */
class VerifyPrimaryTileset {
  public:
    explicit VerifyPrimaryTileset(gsl::not_null<TilesetRepo *> tileset_repo) : tileset_repo_{tileset_repo} {}

    [[nodiscard]] ChainableResult<void> verify(const std::string &tileset_name) const;

  private:
    TilesetRepo *tileset_repo_;
};

} // namespace porytiles2

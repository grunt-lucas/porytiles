#pragma once

#include <memory>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Service that compiles a primary Tileset.
 */
class PrimaryTilesetCompiler {
  public:
    PrimaryTilesetCompiler() = default;

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> compile(const Tileset &tileset);

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> compile_incremental(const Tileset &tileset);
};

} // namespace porytiles2

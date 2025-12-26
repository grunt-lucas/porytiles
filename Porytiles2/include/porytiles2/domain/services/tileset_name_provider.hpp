#pragma once

#include <set>
#include <string>

#include "porytiles2/domain/models/tileset_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

class TilesetNameProvider {
  public:
    virtual ~TilesetNameProvider() = default;

    [[nodiscard]] virtual ChainableResult<std::set<TilesetName>> all_tileset_names() const = 0;
};

} // namespace porytiles2

#pragma once

#include "../templates/any_map.hpp"
#include "../templates/result.hpp"

namespace porytiles {

class Operation {
  public:
    virtual ~Operation() = default;

    virtual Result<AnyMap, BinaryStatus> execute(const AnyMap &inputs) const = 0;
};

} // namespace porytiles

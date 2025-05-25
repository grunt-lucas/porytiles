#include "operations/operation.hpp"

#include "templates/any_map.hpp"
#include "templates/result.hpp"

namespace porytiles {

Result<AnyMap, BinaryStatus> Operation::Execute(const AnyMap &inputs) const {
    return Result<AnyMap, BinaryStatus>{{}};
}

} // namespace porytiles
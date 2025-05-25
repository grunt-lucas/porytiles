#include "operations/operation.hpp"

#include "panic/panic.hpp"
#include "templates/any_map.hpp"
#include "templates/result.hpp"

namespace porytiles {

Result<AnyMap, BinaryStatus> Operation::Execute(const AnyMap &inputs) const {
    for (auto &artifact : Dependencies()) {
        if (!inputs.Contains(artifact.key())) {
            Panic("Missing key: " + artifact.key());
        }
        if (const auto &anyVal = inputs.GetAny(artifact.key());
            std::type_index(anyVal.type()) != artifact.expected_type()) {
            Panic("Type mismatch for key: " + artifact.key());
        }
    }
    return Run(inputs);
}

} // namespace porytiles
#include "porytiles2/domain/models/metatile_attribute.hpp"

#include <string>

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2::attr {

std::string to_string(LayerType layerType)
{
    switch (layerType) {
    case LayerType::normal:
        return "Normal - Middle/Top";
    case LayerType::covered:
        return "Covered - Bottom/Middle";
    case LayerType::split:
        return "Split - Bottom/Top";
    default:
        panic("to_string(LayerType) unknown LayerType");
    }
}

ChainableResult<LayerType> layer_type_from_int(std::uint8_t i)
{
    if (i > static_cast<std::uint8_t>(LayerType::split)) {
        return FormattableError{
            "invalid layer type integer value '{}': must be 0, 1, or 2", FormatParam{i, Style::bold}};
    }
    return static_cast<LayerType>(i);
}

} // namespace porytiles2::attr

#pragma once

#include <cstddef>

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

namespace attr {

/*
 * TODO: remove these hardcoded constants. fieldmap.c and global.fieldmap.h contain definitions for attribute shifts and
 * masks that could be used to infer these values
 */
constexpr std::size_t bytes_per_attr_emerald = 2;
constexpr std::size_t bytes_per_attr_firered = 4;

enum class LayerType : std::uint8_t { normal = 0, covered = 1, split = 2 };

inline std::string to_string(LayerType layerType)
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

} // namespace attr

class MetatileAttribute {
  public:
    MetatileAttribute() = default;

    MetatileAttribute(attr::LayerType layerType, std::uint16_t behavior) : layer_type_{layerType}, behavior_{behavior}
    {
    }

    [[nodiscard]] attr::LayerType layer_type() const
    {
        return layer_type_;
    }

    [[nodiscard]] std::uint16_t behavior() const
    {
        return behavior_;
    }

  private:
    attr::LayerType layer_type_;
    std::uint16_t behavior_;
};

} // namespace porytiles2
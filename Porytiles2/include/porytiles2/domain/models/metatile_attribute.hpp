#pragma once

#include <cstddef>
#include <string>

#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

namespace attr {

/*
 * TODO: remove these hardcoded constants. fieldmap.c and global.fieldmap.h contain definitions for attribute shifts and
 * masks that could be used to infer these values
 */
constexpr std::size_t bytes_per_attr_emerald = 2;
constexpr std::size_t bytes_per_attr_firered = 4;

enum class LayerType : std::uint8_t { normal = 0, covered = 1, split = 2 };

[[nodiscard]] std::string to_string(LayerType layerType);

[[nodiscard]] ChainableResult<LayerType> layer_type_from_int(std::uint8_t i);

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
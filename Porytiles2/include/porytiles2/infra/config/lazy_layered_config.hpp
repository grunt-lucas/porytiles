#pragma once

#include <vector>

#include "porytiles2/domain/config/config.hpp"

namespace porytiles2 {

template <typename T>
struct LayeredValue {
    std::vector<T> values_;
    std::vector<std::string> source_data_;
};

/**
 * @brief A Config implementation that lazily pulls a config value by consulting multiple priority-ordered backing
 * sources.
 */
class LazyLayeredConfig final : public Config {};

} // namespace porytiles2

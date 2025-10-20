#pragma once

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

enum class IncrementalBuildMode { off, keep_unused, remove_unused };

[[nodiscard]] inline std::string to_string(const IncrementalBuildMode &value)
{
    switch (value) {
    case IncrementalBuildMode::off:
        return "off";
    case IncrementalBuildMode::keep_unused:
        return "keep_unused";
    case IncrementalBuildMode::remove_unused:
        return "remove_unused";
    }
    panic("unhandled IncrementalBuildMode value");
}

} // namespace porytiles2

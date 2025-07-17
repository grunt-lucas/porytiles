#pragma once

#include <optional>

namespace porytiles2 {

enum class IncrementalMode { keep_unused, remove_unused };

struct TilesetSettings {
    std::optional<IncrementalMode> incremental;
};

} // namespace porytiles2

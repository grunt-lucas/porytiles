#pragma once

#include <optional>
#include <string>

#include <porytiles2/templates/panic.hpp>

namespace porytiles {

enum class TilesPalMode { kTrueColor, kGreyscale };

[[nodiscard]] inline std::optional<TilesPalMode> TilesPalModeFromStr(const std::string &str) {
    if (str == "true-color") {
        return std::optional{TilesPalMode::kTrueColor};
    }
    if (str == "greyscale") {
        return std::optional{TilesPalMode::kGreyscale};
    }
    return std::nullopt;
}

[[nodiscard]] inline std::string TilesPalModeToStr(const TilesPalMode m) {
    switch (m) {
    case TilesPalMode::kTrueColor:
        return "true-color";
    case TilesPalMode::kGreyscale:
        return "greyscale";
    }
    Panic("unhandled OutputPalette value");
}

inline std::ostream &operator<<(std::ostream &os, const TilesPalMode m) {
    return os << TilesPalModeToStr(m);
}

} // namespace porytiles

#pragma once

#include <optional>
#include <string>

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

enum class TilesPalMode { true_color, greyscale };

[[nodiscard]] inline std::optional<TilesPalMode> tiles_pal_mode_from_str(const std::string &str) {
  if (str == "true-color") {
    return std::optional{TilesPalMode::true_color};
  }
  if (str == "greyscale") {
    return std::optional{TilesPalMode::greyscale};
  }
  return std::nullopt;
}

[[nodiscard]] inline std::string tiles_pal_mode_to_str(const TilesPalMode m) {
  switch (m) {
  case TilesPalMode::true_color:
    return "true-color";
  case TilesPalMode::greyscale:
    return "greyscale";
  }
  panic("unhandled OutputPalette value");
}

inline std::ostream &operator<<(std::ostream &os, const TilesPalMode m) {
  return os << tiles_pal_mode_to_str(m);
}

} // namespace porytiles2

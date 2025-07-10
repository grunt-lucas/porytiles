#pragma once

#include <cstdint>
#include <string>

namespace porytiles {

class Bgr15 {
  std::uint8_t red_;
  std::uint8_t green_;
  std::uint8_t blue_;

public:
  constexpr Bgr15() : red_{0}, green_{0}, blue_{0} {}

  constexpr Bgr15(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) {
    // Class invariant: color channels always have the 3 LSBs unset
    red_ = (red >> 3) << 3;
    green_ = (green >> 3) << 3;
    blue_ = (blue >> 3) << 3;
  }

  [[nodiscard]] std::uint8_t red() const { return red_; }

  [[nodiscard]] std::uint8_t green() const { return green_; }

  [[nodiscard]] std::uint8_t blue() const { return blue_; }

  auto operator<=>(const Bgr15 &) const = default;

  [[nodiscard]] std::uint16_t pack() const;

  [[nodiscard]] std::string to_jasc_str() const;

  [[nodiscard]] static Bgr15 unpack(std::uint16_t packed_bgr);

  // friend std::ostream &operator<<(std::ostream &os, const Bgr15 &bgr);
};

/// Provide a simple way for fmtlib to format Bgr15:
/// https://fmt.dev/11.1/api/#formatting-user-defined-types
inline auto format_as(const Bgr15 &bgr) { return bgr.to_jasc_str(); }

} // namespace porytiles

template <> struct std::hash<porytiles::Bgr15> {
  std::size_t operator()(const porytiles::Bgr15 &bgr) const noexcept {
    return std::hash<uint16_t>{}(bgr.pack());
  }
};

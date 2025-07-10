#include "porytiles2/domain/model/valueobj/bgr15.hpp"

namespace porytiles2 {

std::string Bgr15::to_jasc_str() const {
  return std::to_string(red_) + " " + std::to_string(green_) + " " + std::to_string(blue_);
}

std::uint16_t Bgr15::pack() const {
  return static_cast<std::uint16_t>((blue_ >> 3) << 10 | (green_ >> 3) << 5 | (red_ >> 3));
}

Bgr15 Bgr15::unpack(const std::uint16_t packed_bgr) {
  const std::uint8_t red = (packed_bgr & 0x1f) << 3;
  const std::uint8_t green = (packed_bgr >> 5 & 0x1f) << 3;
  const std::uint8_t blue = (packed_bgr >> 10 & 0x1f) << 3;
  return Bgr15{red, green, blue};
}

} // namespace porytiles2

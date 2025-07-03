#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"

#include "porytiles2/templates/Panic.hpp"

namespace porytiles {

RgbaImage::RgbaImage(const std::size_t width, const std::size_t height) {
  width_ = width;
  height_ = height;
  pixels_.resize(width * height);
}

Rgba32 RgbaImage::At(std::size_t i) const {
  if (constexpr auto size = width_ * height_; i >= size) {
    Panic(fmt::format("index {} out of bounds for image size {}", i, size));
  }

  return pixels_[i];
}

Rgba32 RgbaImage::At(std::size_t row, std::size_t col) const {
  if (col >= width_) {
    Panic(fmt::format("col {} out of bounds for image width {}", col, width_));
  }
  if (row >= height_) {
    Panic(fmt::format("row {} out of bounds for image height {}", row, height_));
  }

  return pixels_[row * width_ + col];
}

void RgbaImage::Set(std::size_t i, const Rgba32 &pixel) {
  if (constexpr auto size = width_ * height_; i >= size) {
    Panic(fmt::format("index {} out of bounds for image size {}", i, size));
  }

  pixels_[i] = pixel;
}

void RgbaImage::Set(std::size_t row, std::size_t col, const Rgba32 &pixel) {
  if (col >= width_) {
    Panic(fmt::format("col {} out of bounds for image width {}", col, width_));
  }
  if (row >= height_) {
    Panic(fmt::format("row {} out of bounds for image height {}", row, height_));
  }

  pixels_[row * width_ + col] = pixel;
}

} // namespace porytiles

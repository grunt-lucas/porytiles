#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"

#include "porytiles2/templates/Panic.hpp"

namespace porytiles2 {

RgbaImage::RgbaImage(const std::size_t width, const std::size_t height) {
  width_ = width;
  height_ = height;
  pixels_.resize(width * height);
}

Rgba32 RgbaImage::at(std::size_t i) const {
  if (const auto size = width_ * height_; i >= size) {
    panic(fmt::format("index {} out of bounds for image size {}", i, size));
  }

  return pixels_[i];
}

Rgba32 RgbaImage::at(std::size_t row, std::size_t col) const {
  if (col >= width_) {
    panic(fmt::format("col {} out of bounds for image width {}", col, width_));
  }
  if (row >= height_) {
    panic(fmt::format("row {} out of bounds for image height {}", row, height_));
  }

  return pixels_[row * width_ + col];
}

void RgbaImage::set(std::size_t i, const Rgba32 &pixel) {
  if (const auto size = width_ * height_; i >= size) {
    panic(fmt::format("index {} out of bounds for image size {}", i, size));
  }

  pixels_[i] = pixel;
}

void RgbaImage::set(std::size_t row, std::size_t col, const Rgba32 &pixel) {
  if (col >= width_) {
    panic(fmt::format("col {} out of bounds for image width {}", col, width_));
  }
  if (row >= height_) {
    panic(fmt::format("row {} out of bounds for image height {}", row, height_));
  }

  pixels_[row * width_ + col] = pixel;
}

} // namespace porytiles2

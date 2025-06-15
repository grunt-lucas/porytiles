#pragma once

#include <expected>
#include <filesystem>

#include <CImg.h>

#include <porytiles2/png/png.hpp>

namespace porytiles {

/**
 * @brief Implementation of Png using the CImg image processing library.
 */
class CImgPng final : public Png {
  public:
    CImgPng() = default;

    [[nodiscard]] std::expected<void, std::string> Read(const std::filesystem::path &path) override;

    [[nodiscard]] std::expected<void, std::string> Write(const std::filesystem::path &path) override;

    void Reset(std::size_t width, std::size_t height) override;

    [[nodiscard]] std::size_t Width() const override;

    [[nodiscard]] std::size_t Height() const override;

  private:
    cimg_library::CImg<std::uint8_t> image_;
};

} // namespace porytiles

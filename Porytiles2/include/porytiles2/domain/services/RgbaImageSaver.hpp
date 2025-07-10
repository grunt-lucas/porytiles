#pragma once

#include <filesystem>

#include "../model/valueobj/RgbaImage.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Service interface for saving RgbaImage instances to various formats.
 *
 * @details
 * This interface follows Domain-Driven Design principles by providing a domain service for saving
 * RgbaImage value objects. Concrete implementations can support different image formats and saving
 * mechanisms.
 */
class RgbaImageSaver {
public:
  virtual ~RgbaImageSaver() = default;

  /**
   * @brief Saves an RgbaImage to a file at the given path.
   *
   * @details
   * If the given path cannot be written to or the image format is not supported, it returns an
   * error message. Otherwise, the operation succeeds and returns a success result.
   *
   * @param image The RgbaImage to save.
   * @param path The path where the image file should be saved.
   * @return A Result indicating success or containing an error description.
   */
  [[nodiscard]] virtual Result<void> save_to_file(const RgbaImage &image,
                                                  const std::filesystem::path &path) const = 0;
};

} // namespace porytiles
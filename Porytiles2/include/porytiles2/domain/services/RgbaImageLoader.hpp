#pragma once

#include <filesystem>
#include <memory>

#include "../model/valueobj/RgbaImage.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles2 {

/**
 * @brief Service interface for loading RgbaImage instances from various sources.
 *
 * @details
 * This interface follows Domain-Driven Design principles by providing a domain service for loading
 * RgbaImage value objects. Concrete implementations can support different image formats and loading
 * mechanisms.
 */
class RgbaImageLoader {
public:
  virtual ~RgbaImageLoader() = default;

  /**
   * @brief Loads an RgbaImage from a file at the given path.
   *
   * @details
   * If the given path does not exist or does not describe a file in a supported format, it returns
   * an error message. Otherwise, the initialized RgbaImage is returned.
   *
   * @param path The path of the image file to load.
   * @return An RgbaImage Result on success, otherwise an error description.
   */
  [[nodiscard]] virtual Result<std::unique_ptr<RgbaImage>>
  load_from_file(const std::filesystem::path &path) const = 0;
};

} // namespace porytiles2
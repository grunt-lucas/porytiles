#pragma once

#include <expected>
#include <filesystem>
#include <memory>

#include "../model/valueobj/RgbaImage.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

/**
 * @brief Repository interface for importing an RgbaImage from the filesystem.
 */
class RgbaImageRepo {
public:
  virtual ~RgbaImageRepo() = default;

  /**
   * @brief Imports an RgbaImage at a given path from the filesystem.
   *
   * @details
   * If the given path does not exist or does not describe a file in a valid
   * format, it returns an error message. Otherwise, the initialized RgbaImage
   * is returned.
   *
   * @param path The path of the RgbaImage to import.
   * @return An RgbaImage Result on success, otherwise an error description.
   */
  [[nodiscard]] virtual Result<std::unique_ptr<RgbaImage>>
  Read(const std::filesystem::path &path) const = 0;
};

} // namespace porytiles

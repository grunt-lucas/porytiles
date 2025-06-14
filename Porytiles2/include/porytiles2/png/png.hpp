#pragma once

#include <expected>
#include <string>

namespace porytiles {

/**
 * @brief An image in PNG format.
 */
class Png {
  public:
    virtual ~Png() = default;

    [[nodiscard]] virtual std::expected<void, std::string> Read(const std::filesystem::path &path) = 0;

    [[nodiscard]] virtual std::expected<void, std::string> Write(const std::filesystem::path &path) = 0;

    [[nodiscard]] virtual std::size_t Width() const = 0;

    [[nodiscard]] virtual std::size_t Height() const = 0;
};

} // namespace porytiles

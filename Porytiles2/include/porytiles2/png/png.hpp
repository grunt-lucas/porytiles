#pragma once

#include <expected>
#include <string>

namespace porytiles {

/**
 * @brief An image in PNG format.
 *
 * @details
 * Clients who need to operate on PNG images can use this interface to avoid dependencies on any particular PNG library.
 * This interface provides a way for clients to read PNGs from the filesystem, manipulate them using some common
 * operations, then write them back to the filesystem.
 */
class Png {
  public:
    virtual ~Png() = default;

    [[nodiscard]] virtual std::expected<void, std::string> Read(const std::filesystem::path &path) = 0;

    [[nodiscard]] virtual std::expected<void, std::string> Write(const std::filesystem::path &path) = 0;

    virtual void Reset(std::size_t width, std::size_t height) = 0;

    /**
     * Gets the width of this Png in pixels.
     * @return The width of this Png in pixels.
     */
    [[nodiscard]] virtual std::size_t Width() const = 0;

    /**
     * Gets the height of this Png in pixels.
     * @return The height of this Png in pixels.
     */
    [[nodiscard]] virtual std::size_t Height() const = 0;
};

} // namespace porytiles

#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include <porytiles2/infra/png/png.hpp>

namespace porytiles {

/**
 * @brief Imports a Png from the filesystem.
 */
class PngImporter {
  public:
    virtual ~PngImporter() = default;

    /**
     * @brief Imports a Png at a given path from the filesystem.
     *
     * @details
     * If the given path does not exist or does not describe a file
     * in valid PNG format, it returns an error message. Otherwise, the initialized Png is returned.
     *
     * @param path The path of the Png to import.
     * @return A std::unique_ptr to the imported Png, otherwise a std::string describing the error.
     */
    [[nodiscard]] virtual std::expected<std::unique_ptr<Png>, std::string>
    Read(const std::filesystem::path &path) const = 0;
};

} // namespace porytiles

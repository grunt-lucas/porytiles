#include "porytiles2/infra/services/png_indexed_image_loader.hpp"

#include <expected>

#include "png++/png.hpp"

#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<std::unique_ptr<Image<std::uint8_t>>>
PngIndexedImageLoader::load_from_file(const std::filesystem::path &path) const {
    return std::unexpected{"foo"};
}

} // namespace porytiles2

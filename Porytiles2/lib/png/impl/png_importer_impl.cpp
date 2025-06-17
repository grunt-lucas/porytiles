#include <png/impl/png_importer_impl.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include <CImg.h>

#include <png/impl/png_impl.hpp>
#include <porytiles2/png/png.hpp>

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

std::expected<std::unique_ptr<Png>, std::string> PngImporterImpl::Read(const std::filesystem::path &path) const {
    CImg<std::uint8_t> image{};
    const auto path_c_str = path.c_str();
    try {
        image.assign(path_c_str);
        return std::make_unique<PngImpl>(image);
    } catch (const CImgException &e) {
        return std::unexpected{e.what()};
    }
}

} // namespace porytiles
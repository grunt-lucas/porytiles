#include "porytiles2/infra/services/jasc_pal_saver.hpp"

#include <fstream>

#include "fmt/format.h"

namespace porytiles2 {

Result<void> JascPalSaver::save(const RgbaPal &pal, const std::filesystem::path &path) const
{
    // Open in binary so "\r\n" are explicitly written as CRLF on all platforms
    std::ofstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return std::unexpected{fmt::format("failed to open file for writing: {}", path.string())};
    }

    stream << "JASC-PAL\r\n";
    stream << "0100\r\n";
    stream << pal.size() << "\r\n";

    for (const auto &color : pal.colors()) {
        stream << color.to_jasc_str() << "\r\n";
    }

    if (stream.fail()) {
        return std::unexpected{fmt::format("failed to write to file: {}", path.string())};
    }

    return {};
}

} // namespace porytiles2

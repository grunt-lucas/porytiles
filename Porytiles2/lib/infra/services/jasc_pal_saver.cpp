#include "porytiles2/infra/services/jasc_pal_saver.hpp"

#include <fstream>

#include "fmt/format.h"

namespace porytiles2 {

Result<void> JascPalSaver::save(const RgbaPal &pal, const std::filesystem::path &path) const
{
    // Open in binary so "\n" are explicitly written as Unix LF
    std::ofstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return std::unexpected{fmt::format("failed to open file for writing: {}", path.string())};
    }

    stream << "JASC-PAL\n";
    stream << "0100\n";
    stream << pal.size() << '\n';

    for (const auto &color : pal.colors()) {
        stream << color.to_jasc_str() << '\n';
    }

    if (stream.fail()) {
        return std::unexpected{fmt::format("failed to write to file: {}", path.string())};
    }

    return {};
}

} // namespace porytiles2

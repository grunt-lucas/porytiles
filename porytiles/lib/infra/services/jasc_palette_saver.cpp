#include "porytiles/infra/services/jasc_palette_saver.hpp"

#include <fstream>

#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

ChainableResult<void>
JascPaletteSaver::save(const Palette<Rgba32, palette::max_size> &palette, const std::filesystem::path &path) const
{
    // Open in binary so "\r\n" are explicitly written as CRLF on all platforms
    std::ofstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return FormattableError{"'{}': Failed to open for writing.", FormatParam{path.string(), Style::bold}};
    }

    // JASC-PAL spec uses \r\n, matching vanilla pokeemerald. Could be made configurable if LF output is ever requested.
    stream << "JASC-PAL\r\n";
    stream << "0100\r\n";
    stream << palette.size() << "\r\n";

    for (std::size_t i = 0; i < palette.size(); i++) {
        if (palette.is_wildcard(i)) {
            stream << "*\r\n";
        }
        else {
            stream << palette.at(i).to_jasc_str() << "\r\n";
        }
    }

    if (stream.fail()) {
        return FormattableError{"'{}': Failed to write.", FormatParam{path.string(), Style::bold}};
    }

    return {};
}

} // namespace porytiles

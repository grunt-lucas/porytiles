#include "porytiles2/infra/services/jasc_pal_saver.hpp"

#include <fstream>

#include "fmt/format.h"

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void>
JascPalSaver::save(const Palette<Rgba32, pal::max_size> &pal, const std::filesystem::path &path) const
{
    // Open in binary so "\r\n" are explicitly written as CRLF on all platforms
    std::ofstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return FormattableError{fmt::format("{}: failed to open for writing", path.string())};
    }

    // TODO: \r\n or \n should be configurable

    stream << "JASC-PAL\r\n";
    stream << "0100\r\n";
    stream << pal.size() << "\r\n";

    for (std::size_t i = 0; i < pal.size(); i++) {
        if (pal.is_wildcard(i)) {
            stream << "*\r\n";
        }
        else {
            stream << pal.at(i).to_jasc_str() << "\r\n";
        }
    }

    if (stream.fail()) {
        return FormattableError{fmt::format("{}: failed to write", path.string())};
    }

    return {};
}

} // namespace porytiles2

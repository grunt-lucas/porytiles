#include "porytiles2/infra/services/jasc_pal_saver.hpp"

#include <fstream>
#include <ranges>

#include "fmt/format.h"

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void> JascPalSaver::save(const Palette<Rgba32> &pal, const std::filesystem::path &path) const
{
    // Open in binary so "\r\n" are explicitly written as CRLF on all platforms
    std::ofstream stream{path, std::ios::binary};
    if (!stream.is_open()) {
        return FormattableError{fmt::format("{}: failed to open for writing", path.string())};
    }

    stream << "JASC-PAL\r\n";
    stream << "0100\r\n";
    stream << pal.size() << "\r\n";

    if (pal.size() >= 1) {
        // First, write slot 0 color
        stream << pal.slot_zero_color().to_jasc_str() << "\r\n";

        // Then write remaining colors (indices 1 through size-1) in order
        const auto index_to_color = pal.index_to_color_map();
        for (const auto &color : index_to_color | std::views::values) {
            stream << color.to_jasc_str() << "\r\n";
        }
    }

    if (stream.fail()) {
        return FormattableError{fmt::format("{}: failed to write", path.string())};
    }

    return {};
}

} // namespace porytiles2

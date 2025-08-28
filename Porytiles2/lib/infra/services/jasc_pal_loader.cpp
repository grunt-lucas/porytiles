#include "porytiles2/infra/services/jasc_pal_loader.hpp"

#include <expected>
#include <filesystem>
#include <fstream>

#include "fmt/format.h"
#include "porytiles2/templates/parsing.hpp"

#include "porytiles2/templates/result.hpp"
#include "porytiles2/templates/string_utils.hpp"

namespace {

using namespace porytiles2;

Result<Rgba32> parse_jasc_line(std::string_view line)
{
    // TODO: implement
    return std::unexpected{"TODO: implement"};
}

} // namespace

namespace porytiles2 {

Result<RgbaPal> JascPalLoader::load(std::filesystem::path &path) const
{
    if (!exists(path)) {
        return std::unexpected{fmt::format("does not exist: {}", path.string())};
    }

    RgbaPal pal{};
    std::string line_buf{};
    std::ifstream stream{path};

    // First line of file *must* be "JASC-PAL"
    std::getline(stream, line_buf);
    trim_line_ending(line_buf);
    if (line_buf != "JASC-PAL") {
        return std::unexpected{fmt::format("{}: expected 'JASC-PAL' on line 1, saw '{}'", path.c_str(), line_buf)};
    }

    // Next line of file *must* be "0100"
    std::getline(stream, line_buf);
    trim_line_ending(line_buf);
    if (line_buf != "0100") {
        return std::unexpected{fmt::format("{}: expected '0100' on line 2, saw '{}'", path.c_str(), line_buf)};
    }

    // Next line of file *must* be the declared size of the palette
    std::getline(stream, line_buf);
    const auto declared_size_result = parse_int<int>(line_buf);
    if (!declared_size_result.has_value()) {
        return std::unexpected{fmt::format(
            "{}: expected integral value on line 3: {}", path.c_str(), line_buf, declared_size_result.error())};
    }
    const auto declared_size = declared_size_result.value();
    if (declared_size < 1) {
        return std::unexpected{fmt::format("{}: expected declared size >= 1, saw '{}'", path.c_str(), declared_size)};
    }

    // Rest of file lines are the colors
    unsigned int color_index = 0;
    while (std::getline(stream, line_buf)) {
        const auto color_result = parse_jasc_line(line_buf);
        if (!color_result.has_value()) {
            return std::unexpected{fmt::format(
                "{}: error parsing color on line {}: {}", path.c_str(), color_index + 4, color_result.error())};
        }
        pal.add(color_result.value());
        color_index++;
    }

    if (color_index != declared_size) {
        return std::unexpected{fmt::format(
            "{}: declared size was '{}' but only saw {} defined colors", path.c_str(), declared_size, color_index)};
    }

    return pal;
}

} // namespace porytiles2

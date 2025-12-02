#include "porytiles2/infra/services/jasc_pal_loader.hpp"

#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>

#include "fmt/format.h"

#include "porytiles2/utilities/parse_int.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

bool is_wildcard_marker(std::string_view line)
{
    return line == "*";
}

ChainableResult<std::optional<Rgba32>> parse_jasc_line(std::string_view line)
{
    if (is_wildcard_marker(line)) {
        return std::optional<Rgba32>{std::nullopt};
    }

    std::vector<std::string> color_components = split(std::string{line}, " ");

    if (color_components.size() != 3) {
        return FormattableError{fmt::format("invalid JASC color format, expected line in format 'R G B'")};
    }

    auto red_result = parse_int<int>(color_components[0]);
    auto green_result = parse_int<int>(color_components[1]);
    auto blue_result = parse_int<int>(color_components[2]);

    if (!red_result.has_value()) {
        return FormattableError{fmt::format("invalid rgb red component: {}", red_result.error())};
    }
    if (!green_result.has_value()) {
        return FormattableError{fmt::format("invalid rgb green component: {}", green_result.error())};
    }
    if (!blue_result.has_value()) {
        return FormattableError{fmt::format("invalid rgb blue component: {}", blue_result.error())};
    }

    const auto red = red_result.value();
    const auto green = green_result.value();
    const auto blue = blue_result.value();

    if (red < 0 || red > 255) {
        return FormattableError{fmt::format("invalid rgb red component '{}': range must be 0 <= red <= 255", red)};
    }

    if (green < 0 || green > 255) {
        return FormattableError{
            fmt::format("invalid rgb green component '{}': range must be 0 <= green <= 255", green)};
    }

    if (blue < 0 || blue > 255) {
        return FormattableError{fmt::format("invalid rgb blue component '{}': range must be 0 <= blue <= 255", blue)};
    }

    return std::optional{Rgba32{
        static_cast<std::uint8_t>(red),
        static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue),
        Rgba32::alpha_opaque}};
}

ChainableResult<Palette<Rgba32, pal::max_size>> parse_jasc_file(const std::filesystem::path &path, bool allow_wildcards)
{
    if (!exists(path)) {
        return FormattableError{fmt::format("does not exist: {}", path.string())};
    }

    Palette<Rgba32, pal::max_size> pal{};
    std::string line_buf{};
    std::ifstream stream{path};

    // TODO: we should print nice line-highlighted error messages like the YamlFileProvider

    // First line of file *must* be "JASC-PAL"
    std::getline(stream, line_buf);
    trim_line_ending(line_buf);
    if (line_buf != "JASC-PAL") {
        return FormattableError{fmt::format("{}: expected 'JASC-PAL' on line 1, saw '{}'", path.c_str(), line_buf)};
    }

    // Next line of file *must* be "0100"
    std::getline(stream, line_buf);
    trim_line_ending(line_buf);
    if (line_buf != "0100") {
        return FormattableError{fmt::format("{}: expected '0100' on line 2, saw '{}'", path.c_str(), line_buf)};
    }

    // Next line of file *must* be the declared size of the palette
    std::getline(stream, line_buf);
    trim_line_ending(line_buf);
    const auto declared_size_result = parse_int<unsigned int>(line_buf);
    if (!declared_size_result.has_value()) {
        return FormattableError{fmt::format(
            "{}: expected integral value on line 3: {}", path.c_str(), line_buf, declared_size_result.error())};
    }
    const auto declared_size = declared_size_result.value();
    if (declared_size != pal::max_size) {
        return FormattableError{
            fmt::format("{}: expected declared size == {}, saw '{}'", path.c_str(), pal::max_size, declared_size)};
    }

    // Rest of file lines are the colors
    unsigned int color_index = 0;
    while (std::getline(stream, line_buf)) {
        const auto trimmed_line = trim_line_ending(line_buf);
        const auto color_result = parse_jasc_line(trimmed_line);
        if (!color_result.has_value()) {
            return ChainableResult<Palette<Rgba32, pal::max_size>>{
                FormattableError{
                    "{}: error parsing color on line {}", FormatParam{path.c_str()}, FormatParam{color_index + 4}},
                color_result};
        }

        const auto &maybe_color = color_result.value();
        if (maybe_color.has_value()) {
            pal.set(maybe_color.value(), color_index);
        }
        else {
            // Wildcard marker encountered
            if (!allow_wildcards) {
                return FormattableError{fmt::format(
                    "{}: wildcard '{}' not allowed on line {} (use load_with_wildcards to allow)",
                    path.c_str(),
                    trimmed_line,
                    color_index + 4)};
            }
            pal.set_wildcard(color_index);
        }
        color_index++;
    }

    if (color_index != declared_size) {
        return FormattableError{fmt::format(
            "{}: declared size was '{}' but only saw {} defined colors", path.c_str(), declared_size, color_index)};
    }

    return pal;
}

} // namespace

namespace porytiles2 {

ChainableResult<Palette<Rgba32, pal::max_size>>
JascPalLoader::load_with_wildcards(const std::filesystem::path &path) const
{
    return parse_jasc_file(path, true);
}

ChainableResult<Palette<Rgba32, pal::max_size>> JascPalLoader::load(const std::filesystem::path &path) const
{
    return parse_jasc_file(path, false);
}

} // namespace porytiles2

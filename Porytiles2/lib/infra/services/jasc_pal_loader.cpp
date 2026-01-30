#include "porytiles2/infra/services/jasc_pal_loader.hpp"

#include <algorithm>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>

#include "porytiles2/utilities/parse_int.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

bool is_wildcard_marker(std::string_view line)
{
    // allow '-' for backwards compatibility with Porytiles1, when writing pals we'll always use '*'
    return line == "*" || line == "-";
}

ChainableResult<std::optional<Rgba32>> parse_jasc_line(std::string_view line, const TextFormatter &format)
{
    if (is_wildcard_marker(line)) {
        return std::optional<Rgba32>{std::nullopt};
    }

    const std::vector<std::string> color_components = split(std::string{line}, " ");

    if (color_components.size() != 3) {
        return FormattableError{format.format(
            "invalid JASC color format, expected line in format '{}'", FormatParam{"R G B", Style::bold})};
    }

    auto red_result = parse_int<int>(color_components[0]);
    auto green_result = parse_int<int>(color_components[1]);
    auto blue_result = parse_int<int>(color_components[2]);

    if (!red_result.has_value()) {
        return FormattableError{format.format(
            "invalid rgb red component '{}': {}", FormatParam{color_components[0], Style::bold}, red_result.error())};
    }
    if (!green_result.has_value()) {
        return FormattableError{format.format(
            "invalid rgb green component '{}': {}",
            FormatParam{color_components[1], Style::bold},
            green_result.error())};
    }
    if (!blue_result.has_value()) {
        return FormattableError{format.format(
            "invalid rgb blue component '{}': {}", FormatParam{color_components[2], Style::bold}, blue_result.error())};
    }

    const auto red = red_result.value();
    const auto green = green_result.value();
    const auto blue = blue_result.value();

    if (red < 0 || red > 255) {
        return FormattableError{format.format(
            "invalid rgb red component '{}': range must be 0 <= red <= 255", FormatParam{red, Style::bold})};
    }

    if (green < 0 || green > 255) {
        return FormattableError{format.format(
            "invalid rgb green component '{}': range must be 0 <= green <= 255", FormatParam{green, Style::bold})};
    }

    if (blue < 0 || blue > 255) {
        return FormattableError{format.format(
            "invalid rgb blue component '{}': range must be 0 <= blue <= 255", FormatParam{blue, Style::bold})};
    }

    return std::optional{Rgba32{
        static_cast<std::uint8_t>(red),
        static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue),
        Rgba32::alpha_opaque}};
}

ChainableResult<Palette<Rgba32, pal::max_size>> parse_jasc_file(
    const std::filesystem::path &path,
    bool allow_wildcards,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer)
{
    if (!exists(path)) {
        return FormattableError{"{}: file does not exist", FormatParam{path.string(), Style::bold}};
    }

    // Slurp the entire file into a vector for FileHighlightPrinter support
    std::vector<std::string> lines{};
    {
        std::ifstream stream{path};
        std::string line_buf{};
        while (std::getline(stream, line_buf)) {
            trim_line_ending(line_buf);
            lines.push_back(line_buf);
        }
    }

    if (lines.empty()) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}: file is empty, expected at least 3 header lines:", FormatParam{path.string(), Style::bold}));
        err_lines.emplace_back();
        err_lines.push_back(format.format("{}", FormatParam{"JASC-PAL", Style::italic}));
        err_lines.push_back(format.format("{}", FormatParam{"0100", Style::italic}));
        err_lines.push_back(format.format("{}", FormatParam{"<PAL-SIZE>", Style::italic}));
        return FormattableError{std::move(err_lines)};
    }

    // Need at least 3 header lines
    if (lines.size() < 3) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}: file too short, expected at least 3 header lines", FormatParam{path.string(), Style::bold}));
        err_lines.emplace_back();
        std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{0}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }

    Palette<Rgba32, pal::max_size> pal{};

    // First line of file *must* be "JASC-PAL"
    if (lines[0] != "JASC-PAL") {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected '{}' as first line of file",
            FormatParam{path.string(), Style::bold},
            FormatParam{"1", Style::bold},
            FormatParam{"JASC-PAL", Style::bold}));
        err_lines.emplace_back();
        std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{0}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }

    // Next line of file *must* be "0100"
    if (lines[1] != "0100") {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected '{}' as second line of file",
            FormatParam{path.string(), Style::bold},
            FormatParam{"2", Style::bold},
            FormatParam{"0100", Style::bold}));
        err_lines.emplace_back();
        std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{1}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }

    // Next line of file *must* be the declared size of the palette
    const auto declared_size_result = parse_int<std::size_t>(lines[2]);
    if (!declared_size_result.has_value()) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected integral value for declared size",
            FormatParam{path.string(), Style::bold},
            FormatParam{"3", Style::bold}));
        err_lines.emplace_back();
        std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{2}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }
    const auto declared_size = declared_size_result.value();
    if (declared_size != pal::max_size) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected declared size to be '{}', saw '{}'",
            FormatParam{path.string(), Style::bold},
            FormatParam{"3", Style::bold},
            FormatParam{pal::max_size, Style::bold},
            FormatParam{declared_size, Style::bold}));
        err_lines.emplace_back();
        std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{2}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }

    // Rest of file lines are the colors (starting at index 3)
    for (std::size_t color_index = 0; color_index < lines.size() - 3; ++color_index) {
        const std::size_t line_index = color_index + 3;
        const auto &line = lines[line_index];
        const auto color_result = parse_jasc_line(line, format);

        if (!color_result.has_value()) {
            std::vector<std::string> err_lines{};
            err_lines.push_back(format.format(
                "{}:{}: error parsing color",
                FormatParam{path.string(), Style::bold},
                FormatParam{line_index + 1, Style::bold}));
            err_lines.emplace_back();
            std::ranges::copy(file_printer.print(lines, std::vector{line_index}), std::back_inserter(err_lines));
            return ChainableResult<Palette<Rgba32, pal::max_size>>{
                FormattableError{std::move(err_lines)}, color_result};
        }

        if (const auto &maybe_color = color_result.value(); maybe_color.has_value()) {
            pal.set(color_index, maybe_color.value());
        }
        else {
            // Wildcard marker encountered
            if (!allow_wildcards) {
                std::vector<std::string> err_lines{};
                err_lines.push_back(format.format(
                    "{}:{}: wildcard not allowed",
                    FormatParam{path.string(), Style::bold},
                    FormatParam{line_index + 1, Style::bold}));
                err_lines.emplace_back();
                std::ranges::copy(file_printer.print(lines, std::vector{line_index}), std::back_inserter(err_lines));
                return FormattableError{std::move(err_lines)};
            }
            pal.set_wildcard(color_index);
        }
    }

    if (const std::size_t actual_color_count = lines.size() - 3; actual_color_count != declared_size) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}: declared size is '{}' but only saw {} defined colors",
            FormatParam{path.string(), Style::bold},
            FormatParam{declared_size, Style::bold},
            FormatParam{actual_color_count}));
        err_lines.emplace_back();
        std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{2}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }

    return pal;
}

} // namespace

namespace porytiles2 {

ChainableResult<Palette<Rgba32, pal::max_size>>
JascPalLoader::load_with_wildcards(const std::filesystem::path &path) const
{
    return parse_jasc_file(path, true, *format_, *file_printer_);
}

ChainableResult<Palette<Rgba32, pal::max_size>> JascPalLoader::load(const std::filesystem::path &path) const
{
    return parse_jasc_file(path, false, *format_, *file_printer_);
}

} // namespace porytiles2

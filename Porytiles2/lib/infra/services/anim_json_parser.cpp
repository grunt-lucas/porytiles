#include "porytiles2/infra/services/anim_json_parser.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "fmt/format.h"
#include "fmt/ranges.h"
#include "nlohmann/json.hpp"

#include "porytiles2/utilities/dynamic_cased_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Converts a byte offset from a JSON parse error into a zero-based line index.
 *
 * @details
 * nlohmann::json reports parse errors with a byte offset. This helper reads the file and counts newlines up to that
 * offset to determine which line the error occurred on.
 *
 * @param json_path Path to the JSON file
 * @param byte_offset The byte offset from the parse error
 * @return Zero-based line index
 */
[[nodiscard]] std::size_t byte_offset_to_line_index(const std::filesystem::path &json_path, std::size_t byte_offset)
{
    std::ifstream in{json_path};
    if (!in) {
        return 0;
    }

    std::size_t line_index = 0;
    std::size_t current_byte = 0;
    char ch{};
    while (in.get(ch) && current_byte < byte_offset) {
        if (ch == '\n') {
            ++line_index;
        }
        ++current_byte;
    }
    return line_index;
}

/**
 * @brief Finds the zero-based line index where a JSON key first appears in the file.
 *
 * @details
 * Scans the file line-by-line looking for the pattern `"key_name"` to approximate where a key is defined. This is used
 * for error reporting with FileHighlightPrinter since nlohmann::json doesn't track source locations.
 *
 * @param json_path Path to the JSON file
 * @param key_name The key to search for
 * @return Zero-based line index, or 0 if not found
 */
[[nodiscard]] std::size_t find_key_line_index(const std::filesystem::path &json_path, const std::string &key_name)
{
    std::ifstream in{json_path};
    if (!in) {
        return 0;
    }

    const std::string pattern = "\"" + key_name + "\"";
    std::string line;
    std::size_t line_index = 0;
    while (std::getline(in, line)) {
        if (line.find(pattern) != std::string::npos) {
            return line_index;
        }
        ++line_index;
    }
    return 0;
}

AnimParams parse_animation_params(const std::string &anim_name, const nlohmann::json &node)
{
    AnimParams params;

    /*
     * TODO: would be nice to warn users here about unknown keys. That way, if they have a simple typo (e.g.
     * 'farme_factor' instead of 'frame_factor') they won't be super confused as to why the animation param isn't being
     * set properly.
     */

    if (node.contains("frame_factor")) {
        params.frame_factor(node["frame_factor"].get<std::size_t>());
    }

    if (node.contains("frame_offset")) {
        params.frame_offset(node["frame_offset"].get<std::size_t>());
    }

    // Parse unique frame definitions
    if (node.contains("frames")) {
        std::vector<DynamicCasedName> frame_names;
        for (const auto &frame : node["frames"]) {
            // Read as string - JSON arrays may contain strings or numbers, convert to string
            frame_names.push_back(DynamicCasedName::from_snake_case(frame.get<std::string>()));
        }
        params.frame_names(std::move(frame_names));
    }

    // Parse playback sequence
    if (node.contains("frame_order")) {
        std::vector<DynamicCasedName> frame_order;
        for (const auto &frame : node["frame_order"]) {
            frame_order.push_back(DynamicCasedName::from_snake_case(frame.get<std::string>()));
        }
        params.frame_order(std::move(frame_order));
    }
    else {
        // Default: frame_order = frame_names (for simple animations where playback order matches definition order)
        params.frame_order(params.frame_names());
    }

    if (node.contains("counter_max")) {
        params.counter_max(node["counter_max"].get<std::size_t>());
    }

    // Note: tile_offset and tile_count are NOT read from anim.json
    // They are computed during compilation and stored in generated_anim_code.h

    params.cased_name(DynamicCasedName{anim_name});
    return params;
}

nlohmann::json serialize_animation_params(const AnimParams &params)
{
    nlohmann::json node;

    // Only write non-default values to keep the file clean
    if (params.frame_factor() != anim::default_frame_factor) {
        node["frame_factor"] = params.frame_factor();
    }

    if (params.frame_offset() != anim::default_frame_offset) {
        node["frame_offset"] = params.frame_offset();
    }

    // Always write frames array since it's the core animation definition
    nlohmann::json frames_array = nlohmann::json::array();
    for (const auto &frame : params.frame_names()) {
        frames_array.push_back(frame.to_snake_case());
    }
    node["frames"] = frames_array;

    // Always write frame_order array since it defines the playback sequence
    nlohmann::json frame_order_array = nlohmann::json::array();
    for (const auto &frame : params.frame_order()) {
        frame_order_array.push_back(frame.to_snake_case());
    }
    node["frame_order"] = frame_order_array;

    if (params.counter_max() != anim::default_counter_max) {
        node["counter_max"] = params.counter_max();
    }

    // Note: tile_offset and tile_count are NOT written to anim.json
    // They are stored in generated_anim_code.h

    return node;
}

} // namespace

namespace porytiles2 {

AnimJsonParser::AnimJsonParser(gsl::not_null<const TextFormatter *> format) : format_{format} {}

ChainableResult<std::map<DynamicCasedName, AnimParams>>
AnimJsonParser::parse(const std::filesystem::path &json_path) const
{
    if (!std::filesystem::exists(json_path)) {
        return FormattableError{
            std::vector<std::string>{"Parameters file not found.", "Expected file: {}"},
            std::vector<std::vector<FormatParam>>{{}, {FormatParam{json_path.string(), Style::bold}}}};
    }

    try {
        std::ifstream in{json_path};
        if (!in) {
            return FormattableError{
                std::vector<std::string>{"Failed to open anim.json for reading.", "path: {}"},
                std::vector<std::vector<FormatParam>>{{}, {FormatParam{json_path.string(), Style::bold}}}};
        }

        nlohmann::json root = nlohmann::json::parse(in);

        std::map<DynamicCasedName, AnimParams> result;

        if (!root.is_object()) {
            FileHighlightPrinter printer{format_};
            std::vector<std::string> err_lines;
            err_lines.push_back(format_->format(
                "{}:1: Invalid anim.json format, expected a JSON object at the root level.",
                FormatParam{json_path.string(), Style::bold}));
            err_lines.emplace_back();
            auto context = printer.print(json_path, std::vector<std::size_t>{0});
            err_lines.insert(err_lines.end(), context.begin(), context.end());
            return FormattableError{std::move(err_lines)};
        }

        for (const auto &[anim_name, anim_node] : root.items()) {
            const std::size_t line_idx = find_key_line_index(json_path, anim_name);

            // Validate snake_case naming convention
            const auto expected_snake = DynamicCasedName{anim_name}.to_snake_case();
            if (expected_snake != anim_name) {
                FileHighlightPrinter printer{format_};
                std::vector<std::string> err_lines;
                err_lines.push_back(format_->format(
                    "{}:{}: Animation name '{}' must be snake_case (expected '{}').",
                    FormatParam{json_path.string(), Style::bold},
                    FormatParam{line_idx + 1},
                    FormatParam{anim_name, Style::bold},
                    FormatParam{expected_snake, Style::bold}));
                err_lines.emplace_back();
                auto context = printer.print(json_path, std::vector{line_idx});
                err_lines.insert(err_lines.end(), context.begin(), context.end());
                return FormattableError{std::move(err_lines)};
            }

            if (!anim_node.is_object()) {
                FileHighlightPrinter printer{format_};
                std::vector<std::string> err_lines;
                err_lines.push_back(format_->format(
                    "{}:{}: Invalid animation entry, '{}' should be a JSON object with frame_factor, frame_offset, "
                    "frames fields.",
                    FormatParam{json_path.string(), Style::bold},
                    FormatParam{line_idx + 1},
                    FormatParam{anim_name, Style::bold}));
                err_lines.emplace_back();
                auto context = printer.print(json_path, std::vector{line_idx});
                err_lines.insert(err_lines.end(), context.begin(), context.end());
                return FormattableError{std::move(err_lines)};
            }

            auto parsed = parse_animation_params(anim_name, anim_node);
            result.insert({DynamicCasedName{anim_name}, std::move(parsed)});

            // Validate that frame_order entries reference valid frame_names
            const auto &parsed_params = result.at(DynamicCasedName{anim_name});
            std::set<DynamicCasedName> valid_frames(
                parsed_params.frame_names().begin(), parsed_params.frame_names().end());

            for (const auto &frame : parsed_params.frame_order()) {
                if (!valid_frames.contains(frame)) {
                    FileHighlightPrinter printer{format_};
                    std::vector<std::string> err_lines;
                    err_lines.push_back(format_->format(
                        "{}:{}: frame_order entry '{}' does not exist in frames list.",
                        FormatParam{json_path.string(), Style::bold},
                        FormatParam{line_idx + 1},
                        FormatParam{frame.to_snake_case(), Style::bold}));
                    err_lines.emplace_back();

                    // Transform frame names to snake_case strings for display
                    std::vector<std::string> frame_strs;
                    frame_strs.reserve(parsed_params.frame_names().size());
                    for (const auto &f : parsed_params.frame_names()) {
                        frame_strs.push_back(f.to_snake_case());
                    }
                    err_lines.push_back(format_->format(
                        "Valid frames are: {}.",
                        FormatParam{fmt::format("[{}]", fmt::join(frame_strs, ", ")), Style::bold}));
                    err_lines.emplace_back();
                    auto context = printer.print(json_path, std::vector<std::size_t>{line_idx});
                    err_lines.insert(err_lines.end(), context.begin(), context.end());
                    return FormattableError{std::move(err_lines)};
                }
            }
        }

        return result;
    }
    catch (const nlohmann::json::parse_error &e) {
        FileHighlightPrinter printer{format_};
        std::vector<std::string> err_lines;

        const auto byte_offset = static_cast<std::size_t>(e.byte);
        const auto line_idx = byte_offset_to_line_index(json_path, byte_offset);
        err_lines.push_back(format_->format(
            "{}:{}: Failed to parse anim.json: {}.",
            FormatParam{json_path.string(), Style::bold},
            FormatParam{line_idx + 1},
            FormatParam{e.what()}));
        err_lines.emplace_back();
        auto context = printer.print(json_path, std::vector<std::size_t>{line_idx});
        err_lines.insert(err_lines.end(), context.begin(), context.end());

        return FormattableError{std::move(err_lines)};
    }
    catch (const nlohmann::json::exception &e) {
        return FormattableError{
            std::vector<std::string>{"{}: Failed to parse anim.json: {}."},
            std::vector<std::vector<FormatParam>>{
                {FormatParam{json_path.string(), Style::bold}, FormatParam{e.what()}}}};
    }
}

ChainableResult<void> AnimJsonParser::write(
    const std::filesystem::path &json_path, const std::map<DynamicCasedName, AnimParams> &params) const
{
    try {
        // Create parent directories if they don't exist
        if (json_path.has_parent_path()) {
            std::filesystem::create_directories(json_path.parent_path());
        }

        nlohmann::json root;
        for (const auto &[name, anim_params] : params) {
            root[name.to_snake_case()] = serialize_animation_params(anim_params);
        }

        std::ofstream out(json_path);
        if (!out) {
            return FormattableError{
                std::vector<std::string>{"Failed to open anim.json for writing.", "path: {}"},
                std::vector<std::vector<FormatParam>>{{}, {FormatParam{json_path.string(), Style::bold}}}};
        }

        out << root.dump(2);
        out << std::endl;
        out.close();

        if (!out) {
            return FormattableError{
                std::vector<std::string>{"Failed to write anim.json.", "Error occurred while writing to: {}"},
                std::vector<std::vector<FormatParam>>{{}, {FormatParam{json_path.string(), Style::bold}}}};
        }

        return {};
    }
    catch (const nlohmann::json::exception &e) {
        return FormattableError{
            std::vector<std::string>{"Failed to serialize anim.json.", "JSON error: {}"},
            std::vector<std::vector<FormatParam>>{{}, {FormatParam{e.what()}}}};
    }
}

} // namespace porytiles2

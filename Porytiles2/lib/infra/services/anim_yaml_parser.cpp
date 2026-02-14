#include "porytiles2/infra/services/anim_yaml_parser.hpp"

#include <fstream>
#include <set>

#include "fmt/format.h"
#include "fmt/ranges.h"
#include "yaml-cpp/yaml.h"

#include "porytiles2/utilities/dynamic_cased_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"

namespace {

using namespace porytiles2;

AnimParams parse_animation_params(const std::string &anim_name, const YAML::Node &node)
{
    AnimParams params;

    /*
     * TODO: would be nice to warn users here about unknown keys. That way, if they have a simple typo (e.g.
     * 'farme_factor' instead of 'frame_factor') they won't be super confused as to why the animation param isn't being
     * set properly.
     */

    if (node["frame_factor"]) {
        params.frame_factor(node["frame_factor"].as<std::size_t>());
    }

    if (node["frame_offset"]) {
        params.frame_offset(node["frame_offset"].as<std::size_t>());
    }

    // Parse unique frame definitions
    if (node["frames"]) {
        std::vector<DynamicCasedName> frame_names;
        for (const auto &frame : node["frames"]) {
            // Read as string - works for both "0" and 0 due to YAML type coercion
            frame_names.push_back(DynamicCasedName::from_snake_case(frame.as<std::string>()));
        }
        params.frame_names(std::move(frame_names));
    }

    // Parse playback sequence
    if (node["frame_order"]) {
        std::vector<DynamicCasedName> frame_order;
        for (const auto &frame : node["frame_order"]) {
            frame_order.push_back(DynamicCasedName::from_snake_case(frame.as<std::string>()));
        }
        params.frame_order(std::move(frame_order));
    }
    else {
        // Default: frame_order = frame_names (for simple animations where playback order matches definition order)
        params.frame_order(params.frame_names());
    }

    if (node["counter_max"]) {
        params.counter_max(node["counter_max"].as<std::size_t>());
    }

    // Note: tile_offset and tile_count are NOT read from anim.yaml
    // They are computed during compilation and stored in generated_anim_code.h

    params.cased_name(DynamicCasedName{anim_name});
    return params;
}

YAML::Node serialize_animation_params(const AnimParams &params)
{
    YAML::Node node;

    // Only write non-default values to keep the file clean
    if (params.frame_factor() != anim::default_frame_factor) {
        node["frame_factor"] = params.frame_factor();
    }

    if (params.frame_offset() != anim::default_frame_offset) {
        node["frame_offset"] = params.frame_offset();
    }

    // Always write frames array since it's the core animation definition
    YAML::Node frames_node;
    frames_node.SetStyle(YAML::EmitterStyle::Flow);
    for (const auto &frame : params.frame_names()) {
        frames_node.push_back(frame.to_snake_case());
    }
    node["frames"] = frames_node;

    // Always write frame_order array since it defines the playback sequence
    YAML::Node frame_order_node;
    frame_order_node.SetStyle(YAML::EmitterStyle::Flow);
    for (const auto &frame : params.frame_order()) {
        frame_order_node.push_back(frame.to_snake_case());
    }
    node["frame_order"] = frame_order_node;

    if (params.counter_max() != anim::default_counter_max) {
        node["counter_max"] = params.counter_max();
    }

    // Note: tile_offset and tile_count are NOT written to anim.yaml
    // They are stored in generated_anim_code.h

    return node;
}

} // namespace

namespace porytiles2 {

AnimYamlParser::AnimYamlParser(gsl::not_null<const TextFormatter *> format) : format_{format} {}

ChainableResult<std::map<DynamicCasedName, AnimParams>>
AnimYamlParser::parse(const std::filesystem::path &yaml_path) const
{
    if (!std::filesystem::exists(yaml_path)) {
        return FormattableError{
            std::vector<std::string>{"Parameters file file not found.", "Expected file: {}"},
            std::vector<std::vector<FormatParam>>{{}, {FormatParam{yaml_path.string(), Style::bold}}}};
    }

    try {
        YAML::Node root = YAML::LoadFile(yaml_path.string());

        std::map<DynamicCasedName, AnimParams> result;

        if (!root.IsMap()) {
            FileHighlightPrinter printer{format_};
            std::vector<std::string> err_lines;
            err_lines.push_back(format_->format(
                "{}:1: Invalid anim.yaml format, expected a YAML map at the root level",
                FormatParam{yaml_path.string(), Style::bold}));
            err_lines.emplace_back();
            auto context = printer.print(yaml_path, std::vector<std::size_t>{0});
            err_lines.insert(err_lines.end(), context.begin(), context.end());
            return FormattableError{std::move(err_lines)};
        }

        for (const auto &entry : root) {
            const auto anim_name = entry.first.as<std::string>();
            const YAML::Node &anim_node = entry.second;
            const auto mark = entry.first.Mark();
            const std::size_t line_idx = mark.line;

            // Validate snake_case naming convention
            const auto expected_snake = DynamicCasedName{anim_name}.to_snake_case();
            if (expected_snake != anim_name) {
                FileHighlightPrinter printer{format_};
                std::vector<std::string> err_lines;
                err_lines.push_back(format_->format(
                    "{}:{}: Animation name '{}' must be snake_case (expected '{}').",
                    FormatParam{yaml_path.string(), Style::bold},
                    FormatParam{line_idx + 1},
                    FormatParam{anim_name, Style::bold},
                    FormatParam{expected_snake, Style::bold}));
                err_lines.emplace_back();
                auto context = printer.print(yaml_path, std::vector{line_idx});
                err_lines.insert(err_lines.end(), context.begin(), context.end());
                return FormattableError{std::move(err_lines)};
            }

            if (!anim_node.IsMap()) {
                FileHighlightPrinter printer{format_};
                std::vector<std::string> err_lines;
                err_lines.push_back(format_->format(
                    "{}:{}: Invalid animation entry, '{}' should be a YAML map with frame_factor, frame_offset, frames "
                    "fields.",
                    FormatParam{yaml_path.string(), Style::bold},
                    FormatParam{line_idx + 1},
                    FormatParam{anim_name, Style::bold}));
                err_lines.emplace_back();
                auto context = printer.print(yaml_path, std::vector{line_idx});
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
                        "{}:{}: frame_order entry '{}' does not exist in frames list",
                        FormatParam{yaml_path.string(), Style::bold},
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
                        "valid frames are: {}",
                        FormatParam{fmt::format("[{}]", fmt::join(frame_strs, ", ")), Style::bold}));
                    err_lines.emplace_back();
                    auto context = printer.print(yaml_path, std::vector<std::size_t>{line_idx});
                    err_lines.insert(err_lines.end(), context.begin(), context.end());
                    return FormattableError{std::move(err_lines)};
                }
            }
        }

        return result;
    }
    catch (const YAML::Exception &e) {
        FileHighlightPrinter printer{format_};
        std::vector<std::string> err_lines;

        const auto &mark = e.mark;
        if (mark.line >= 0) {
            const auto line_idx = static_cast<std::size_t>(mark.line);
            err_lines.push_back(format_->format(
                "{}:{}: Failed to parse anim.yaml: {}",
                FormatParam{yaml_path.string(), Style::bold},
                FormatParam{line_idx + 1},
                FormatParam{e.msg}));
            err_lines.emplace_back();
            auto context = printer.print(yaml_path, std::vector<std::size_t>{line_idx});
            err_lines.insert(err_lines.end(), context.begin(), context.end());
        }
        else {
            err_lines.push_back(format_->format(
                "{}: Failed to parse anim.yaml: {}",
                FormatParam{yaml_path.string(), Style::bold},
                FormatParam{e.what()}));
        }

        return FormattableError{std::move(err_lines)};
    }
}

ChainableResult<void> AnimYamlParser::write(
    const std::filesystem::path &yaml_path, const std::map<DynamicCasedName, AnimParams> &params) const
{
    try {
        // Create parent directories if they don't exist
        if (yaml_path.has_parent_path()) {
            std::filesystem::create_directories(yaml_path.parent_path());
        }

        YAML::Node root;
        for (const auto &[name, anim_params] : params) {
            root[name.to_snake_case()] = serialize_animation_params(anim_params);
        }

        std::ofstream out(yaml_path);
        if (!out) {
            return FormattableError{
                std::vector<std::string>{"Failed to open anim.yaml for writing.", "path: {}"},
                std::vector<std::vector<FormatParam>>{{}, {FormatParam{yaml_path.string(), Style::bold}}}};
        }

        out << root;
        out << std::endl;
        out.close();

        if (!out) {
            return FormattableError{
                std::vector<std::string>{"Failed to write anim.yaml.", "Error occurred while writing to: {}"},
                std::vector<std::vector<FormatParam>>{{}, {FormatParam{yaml_path.string(), Style::bold}}}};
        }

        return {};
    }
    catch (const YAML::Exception &e) {
        return FormattableError{
            std::vector<std::string>{"Failed to serialize anim.yaml.", "YAML error: {}"},
            std::vector<std::vector<FormatParam>>{{}, {FormatParam{e.what()}}}};
    }
}

} // namespace porytiles2
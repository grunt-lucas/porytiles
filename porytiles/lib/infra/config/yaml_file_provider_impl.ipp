#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "yaml-cpp/yaml.h"

#include "porytiles/app/config/primary_pairing_mode.hpp"
#include "porytiles/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_multi_pal_subtile_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles/domain/config/artifact_edit_mode.hpp"
#include "porytiles/domain/config/frame_linking.hpp"
#include "porytiles/domain/config/metatile_attr_field_spec.hpp"
#include "porytiles/domain/config/packing_strategy_params.hpp"
#include "porytiles/domain/config/packing_strategy_type.hpp"
#include "porytiles/domain/config/per_anim_overrides.hpp"
#include "porytiles/domain/config/tile_sharing_alignment.hpp"
#include "porytiles/domain/config/tile_sharing_packing.hpp"
#include "porytiles/domain/config/tiles_pal_mode.hpp"
#include "porytiles/domain/packing/models/palette_hint.hpp"
#include "porytiles/infra/config/config_provider.hpp"
#include "porytiles/infra/config/frlg_alternate_mask_mode.hpp"
#include "porytiles/infra/config/valid_yaml_paths.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

// The anonymous namespace ensures internal linkage per translation unit
// This file is intentionally included only in yaml_file_provider.cpp
namespace {

using namespace porytiles;

// Static caches shared across all YamlFileProvider instances
std::map<std::filesystem::path, YAML::Node> yaml_cache;
std::map<std::filesystem::path, std::vector<std::string>> file_lines_cache;

/**
 * @brief Gets the content of a specific line from a cached YAML file.
 *
 * @details
 * Returns the content of the specified line from the file contents cache. If the file is not cached or the line number
 * is out of bounds, returns an empty string.
 *
 * @param path The path to the YAML file
 * @param line_num The line number (0-indexed)
 * @return The line content, or empty string if not found
 */
std::string get_line_content(const std::filesystem::path &path, std::size_t line_num)
{
    const auto it = file_lines_cache.find(path);
    if (it != file_lines_cache.end() && line_num < it->second.size()) {
        return it->second[line_num];
    }
    return "";
}

/**
 * @brief Constructs a source location string with file path and line number.
 *
 * @details
 * Creates a formatted string in the form "path:line" where:
 * - path is the file path
 * - line is the 1-indexed line number
 *
 * @param format The text formatter to use
 * @param file_path The path to the YAML file
 * @param mark The YAML mark containing line number information
 * @return Formatted source location string
 */
std::string make_source_string(const TextFormatter *format, const std::string &file_path, const YAML::Mark &mark)
{
    return format->format("{}:{}", FormatParam{file_path}, FormatParam{mark.line + 1});
}

/**
 * @brief Constructs source details showing contextual lines around the target line.
 *
 * @details
 * Creates a vector of strings showing a contextual view of the YAML file around the target line. The view includes a
 * configurable number of lines before and after the target line, with the target line marked with a "> " prefix. Each
 * line is formatted with its line number.
 *
 * For example, with window_size=5 and target line 10:
 * ```
 *   8: some_config: value
 *   9: another_config: value
 * > 10: target_line: value
 *   11: next_config: value
 *   12: last_config: value
 * ```
 *
 * If the target line is near the start or end of the file, the window adjusts to show the available lines while
 * maintaining the requested window size where possible.
 *
 * @param file_path The path to the YAML file
 * @param mark The YAML mark containing line number information
 * @return Vector of formatted strings showing the contextual view
 */
std::vector<std::string>
make_source_details(const TextFormatter *format, const std::string &file_path, const YAML::Mark &mark)
{
    const std::filesystem::path path{file_path};
    const auto it = file_lines_cache.find(path);
    if (it == file_lines_cache.end()) {
        return {};
    }

    const auto &lines = it->second;
    const std::size_t line_num = mark.line; // 0-indexed

    if (lines.empty() || line_num >= lines.size()) {
        return {};
    }

    // Use FileHighlightPrinter (line_num is already 0-indexed)
    const FileHighlightPrinter printer{format};
    return printer.print(lines, std::vector{line_num});
}

/**
 * @brief Recursively collects all dot-separated paths from a YAML node.
 *
 * @details
 * Walks the YAML node tree and collects all map keys as dot-separated paths. For each key
 * encountered, stores the full path and the YAML::Mark for source location reporting.
 *
 * @param node The YAML node to traverse
 * @param prefix Current path prefix (empty for root)
 * @param paths Output vector of discovered paths with their marks
 */
void collect_yaml_paths(
    const YAML::Node &node, const std::string &prefix, std::vector<std::pair<std::string, YAML::Mark>> &paths)
{
    if (!node.IsMap()) {
        return;
    }

    for (const auto &kv : node) {
        const auto key = kv.first.as<std::string>();
        const auto full_path = prefix.empty() ? key : prefix + "." + key;
        paths.emplace_back(full_path, kv.first.Mark());

        // Recurse into nested maps
        if (kv.second.IsMap()) {
            collect_yaml_paths(kv.second, full_path, paths);
        }
    }
}

/**
 * @brief Validates YAML paths against the known valid paths set.
 *
 * @details
 * Walks the YAML document tree and compares each path against the set of valid paths
 * defined in valid_yaml_paths.hpp. For any unknown paths, emits an error via the
 * UserDiagnostics interface with source location and context.
 *
 * @param format Text formatter for styled output
 * @param diagnostics User diagnostics for emitting errors (may be nullptr)
 * @param file_path Path to the YAML file being validated
 * @param node The root YAML node to validate
 * @return @c true if any unknown keys were found, @c false otherwise.
 */
[[nodiscard]] bool validate_yaml_paths(
    const TextFormatter *format,
    const UserDiagnostics *diagnostics,
    const std::filesystem::path &file_path,
    const YAML::Node &node)
{
    if (diagnostics == nullptr) {
        return false;
    }

    bool found_unknown = false;
    std::vector<std::pair<std::string, YAML::Mark>> paths;
    collect_yaml_paths(node, "", paths);

    for (const auto &[path, mark] : paths) {
        if (!valid_yaml_paths.contains(path)) {
            // Skip children of map-type config values (dynamic keys like animation names)
            bool is_map_child = false;
            for (const auto &prefix : valid_yaml_map_prefixes) {
                if (path.starts_with(prefix + ".")) {
                    is_map_child = true;
                    break;
                }
            }
            if (is_map_child) {
                continue;
            }

            const auto source = make_source_string(format, file_path.string(), mark);
            auto details = make_source_details(format, file_path.string(), mark);

            std::vector<std::string> error_lines;
            error_lines.push_back(format->format("Unknown configuration key '{}'.", FormatParam{path, Style::bold}));
            error_lines.emplace_back();
            error_lines.push_back(format->format("Source: {}", FormatParam{source, Style::italic}));
            error_lines.emplace_back();
            for (auto &detail : details) {
                error_lines.push_back(std::move(detail));
            }

            diagnostics->error("unknown-config-key", error_lines);
            found_unknown = true;
        }
    }

    return found_unknown;
}

/**
 * @brief Attempts to parse a std::size_t value from a YAML node.
 *
 * @param format The text formatter to use
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
LayerValue<std::size_t>
parse_size_t(const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<std::size_t>::not_provided();
    }

    try {
        const auto value = node.as<std::size_t>();
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<std::size_t>::valid(value, key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as integer: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<std::size_t>::invalid(error, source, details);
    }
}

/**
 * @brief Attempts to parse a bool value from a YAML node.
 *
 * @param format The text formatter to use
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
LayerValue<bool>
parse_bool(const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<bool>::not_provided();
    }

    try {
        const auto value = node.as<bool>();
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<bool>::valid(value, key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as boolean: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<bool>::invalid(error, source, details);
    }
}

/**
 * @brief Attempts to parse a std::string value from a YAML node.
 *
 * @param format The text formatter to use
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
LayerValue<std::string>
parse_string(const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<std::string>::not_provided();
    }

    try {
        const auto value = node.as<std::string>();
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<std::string>::valid(value, key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as string: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<std::string>::invalid(error, source, details);
    }
}

/**
 * @brief Attempts to parse an Rgba32 color from a YAML node.
 *
 * @details
 * Expects the YAML node to be a sequence with 3 or 4 elements: [r, g, b] or [r, g, b, a]. If alpha is not provided, it
 * defaults to 255 (opaque).
 *
 * @param format The text formatter to use
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
LayerValue<Rgba32>
parse_rgba32(const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<Rgba32>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsSequence()) {
            const auto error =
                format->format("'{}' must be a sequence [r, g, b] or [r, g, b, a]", FormatParam{key, Style::bold});
            return LayerValue<Rgba32>::invalid(error, source, details);
        }

        if (node.size() < 3 || node.size() > 4) {
            const auto error = format->format(
                "'{}' must have 3 or 4 elements [r, g, b] or [r, g, b, a], got {}",
                FormatParam{key, Style::bold},
                FormatParam{node.size(), Style::bold});
            return LayerValue<Rgba32>::invalid(error, source, details);
        }

        const auto r = node[0].as<std::uint8_t>();
        const auto g = node[1].as<std::uint8_t>();
        const auto b = node[2].as<std::uint8_t>();
        const auto a = (node.size() == 4) ? node[3].as<std::uint8_t>() : Rgba32::alpha_opaque;

        const Rgba32 color{r, g, b, a};
        return LayerValue<Rgba32>::valid(color, key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format("Failed to parse '{}' as rgba: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<Rgba32>::invalid(error, source, details);
    }
}

LayerValue<std::vector<PaletteHint>> parse_pal_hints(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<std::vector<PaletteHint>>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsSequence()) {
            const auto error =
                format->format("'{}' must be a sequence of palette hints", FormatParam{key, Style::bold});
            const auto source = make_source_string(format, file_path, mark);
            return LayerValue<std::vector<PaletteHint>>::invalid(error, source, details);
        }

        std::vector<PaletteHint> hints;
        for (std::size_t i = 0; i < node.size(); ++i) {
            const auto &hint_node = node[i];

            if (!hint_node.IsMap()) {
                const auto hint_mark = hint_node.Mark();
                const auto error = format->format(
                    "'{}[{}]' must be a map with 'name' and 'colors' keys",
                    FormatParam{key, Style::bold},
                    FormatParam{i, Style::bold});
                const auto source = make_source_string(format, file_path, hint_mark);
                const auto hint_details = make_source_details(format, file_path, hint_mark);
                return LayerValue<std::vector<PaletteHint>>::invalid(error, source, hint_details);
            }

            // Parse name field
            const auto name_node = hint_node["name"];
            if (!name_node.IsDefined()) {
                const auto hint_mark = hint_node.Mark();
                const auto error = format->format(
                    "'{}[{}]' is missing required 'name' field", FormatParam{key, Style::bold}, FormatParam{i});
                const auto source = make_source_string(format, file_path, hint_mark);
                const auto hint_details = make_source_details(format, file_path, hint_mark);
                return LayerValue<std::vector<PaletteHint>>::invalid(error, source, hint_details);
            }
            const auto name = name_node.as<std::string>();

            // Parse colors field
            const auto colors_node = hint_node["colors"];
            if (!colors_node.IsDefined()) {
                const auto hint_mark = hint_node.Mark();
                const auto error = format->format(
                    "'{}[{}]' is missing required 'colors' field", FormatParam{key, Style::bold}, FormatParam{i});
                const auto source = make_source_string(format, file_path, hint_mark);
                const auto hint_details = make_source_details(format, file_path, hint_mark);
                return LayerValue<std::vector<PaletteHint>>::invalid(error, source, hint_details);
            }

            if (!colors_node.IsSequence()) {
                const auto colors_mark = colors_node.Mark();
                const auto error = format->format(
                    "'{}[{}].colors' must be a sequence of colors", FormatParam{key, Style::bold}, FormatParam{i});
                const auto source = make_source_string(format, file_path, colors_mark);
                const auto colors_details = make_source_details(format, file_path, colors_mark);
                return LayerValue<std::vector<PaletteHint>>::invalid(error, source, colors_details);
            }

            // Parse each color
            std::vector<Rgba32> colors;
            for (std::size_t j = 0; j < colors_node.size(); ++j) {
                const auto &color_node = colors_node[j];

                if (!color_node.IsSequence() || color_node.size() != 3) {
                    const auto color_mark = color_node.Mark();
                    const auto error = format->format(
                        "'{}[{}].colors[{}]' must be [r, g, b]",
                        FormatParam{key, Style::bold},
                        FormatParam{i},
                        FormatParam{j});
                    const auto source = make_source_string(format, file_path, color_mark);
                    const auto color_details = make_source_details(format, file_path, color_mark);
                    return LayerValue<std::vector<PaletteHint>>::invalid(error, source, color_details);
                }

                const auto r = color_node[0].as<std::uint8_t>();
                const auto g = color_node[1].as<std::uint8_t>();
                const auto b = color_node[2].as<std::uint8_t>();
                const auto a = (color_node.size() == 4) ? color_node[3].as<std::uint8_t>() : Rgba32::alpha_opaque;

                colors.emplace_back(r, g, b, a);
            }

            hints.emplace_back(name, Palette{std::move(colors)});
        }

        const auto source = make_source_string(format, file_path, mark);
        return LayerValue<std::vector<PaletteHint>>::valid(std::move(hints), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as palette hints: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<std::vector<PaletteHint>>::invalid(error, source, details);
    }
}

/**
 * @brief Parses a std::vector<std::string> from a YAML sequence node.
 *
 * @details
 * Expects a YAML sequence of strings. Returns LayerValue::not_provided() if the node is undefined.
 * Returns LayerValue::invalid() if the node is not a sequence.
 *
 * @param format The text formatter to use
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed vector, error, or not_provided status
 */
LayerValue<std::vector<std::string>> parse_string_vector(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<std::vector<std::string>>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsSequence()) {
            const auto error = format->format("'{}' must be a sequence of strings.", FormatParam{key, Style::bold});
            return LayerValue<std::vector<std::string>>::invalid(error, source, details);
        }

        std::vector<std::string> result;
        for (std::size_t i = 0; i < node.size(); ++i) {
            result.push_back(node[i].as<std::string>());
        }
        return LayerValue<std::vector<std::string>>::valid(std::move(result), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as string list: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<std::vector<std::string>>::invalid(error, source, details);
    }
}

/**
 * @brief Attempts to parse a TilesPalMode value from a YAML node.
 *
 * @details
 * Expects a string value that matches one of the valid TilesPalMode YAML strings: "true_color" or "greyscale".
 *
 * @param format The text formatter to use
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
LayerValue<TilesPalMode> parse_tiles_pal_mode(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<TilesPalMode>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = tiles_pal_mode_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<TilesPalMode>::invalid(error, source, details);
        }

        return LayerValue<TilesPalMode>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as TilesPalMode: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<TilesPalMode>::invalid(error, source, details);
    }
}

LayerValue<ArtifactEditMode> parse_artifact_edit_mode(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<ArtifactEditMode>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = artifact_edit_mode_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<ArtifactEditMode>::invalid(error, source, details);
        }

        return LayerValue<ArtifactEditMode>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as ArtifactEditMode: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<ArtifactEditMode>::invalid(error, source, details);
    }
}

LayerValue<AnimPalResolutionStrategy> parse_anim_pal_resolution_strategy(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<AnimPalResolutionStrategy>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = anim_pal_resolution_strategy_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<AnimPalResolutionStrategy>::invalid(error, source, details);
        }

        return LayerValue<AnimPalResolutionStrategy>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format(
            "Failed to parse '{}' as AnimPalResolutionStrategy: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<AnimPalResolutionStrategy>::invalid(error, source, details);
    }
}

LayerValue<PerAnimOverrides> parse_per_anim_overrides(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<PerAnimOverrides>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsMap()) {
            const auto error = format->format(
                "'{}' must be a map of animation names to config objects.", FormatParam{key, Style::bold});
            return LayerValue<PerAnimOverrides>::invalid(error, source, details);
        }

        PerAnimOverrides configs;
        for (const auto &kv : node) {
            const auto anim_name = kv.first.as<std::string>();
            const auto &anim_node = kv.second;

            PerAnimOverride anim_config;
            anim_config.anim_name = anim_name;

            if (!anim_node.IsMap()) {
                const auto anim_mark = kv.first.Mark();
                const auto anim_source = make_source_string(format, file_path, anim_mark);
                const auto anim_details = make_source_details(format, file_path, anim_mark);
                const auto error = format->format(
                    "'{}' animation '{}' must be a map.",
                    FormatParam{key, Style::bold},
                    FormatParam{anim_name, Style::bold});
                return LayerValue<PerAnimOverrides>::invalid(error, anim_source, anim_details);
            }

            // Parse frame_linking (optional)
            if (anim_node["frame_linking"].IsDefined()) {
                const auto linking_str = anim_node["frame_linking"].as<std::string>();
                const auto linking_opt = frame_linking_from_str(linking_str);
                if (!linking_opt.has_value()) {
                    const auto linking_mark = anim_node["frame_linking"].Mark();
                    const auto linking_source = make_source_string(format, file_path, linking_mark);
                    const auto linking_details = make_source_details(format, file_path, linking_mark);
                    const auto error = format->format(
                        "'{}' animation '{}' has invalid frame_linking value '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{anim_name, Style::bold},
                        FormatParam{linking_str, Style::bold});
                    return LayerValue<PerAnimOverrides>::invalid(error, linking_source, linking_details);
                }
                const auto fl_mark = anim_node["frame_linking"].Mark();
                anim_config.linking = ConfigPODField{
                    linking_opt.value(),
                    key + "." + anim_name + ".frame_linking",
                    "Animation Config (" + anim_name + ") frame_linking",
                    make_source_string(format, file_path, fl_mark),
                    make_source_details(format, file_path, fl_mark)};
            }

            // Parse palette_resolution_strategy (optional scalar — per-anim middle tier)
            if (anim_node["palette_resolution_strategy"].IsDefined()) {
                const auto &strategy_node = anim_node["palette_resolution_strategy"];
                const auto strategy_str = strategy_node.as<std::string>();
                const auto strategy_opt = anim_pal_resolution_strategy_from_str(strategy_str);
                if (!strategy_opt.has_value()) {
                    const auto strategy_mark = strategy_node.Mark();
                    const auto strategy_source = make_source_string(format, file_path, strategy_mark);
                    const auto strategy_details = make_source_details(format, file_path, strategy_mark);
                    const auto error = format->format(
                        "'{}' animation '{}' palette_resolution_strategy has invalid value '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{anim_name, Style::bold},
                        FormatParam{strategy_str, Style::bold});
                    return LayerValue<PerAnimOverrides>::invalid(error, strategy_source, strategy_details);
                }
                const auto pal_mark = strategy_node.Mark();
                anim_config.pal_resolution_strategy = ConfigPODField{
                    strategy_opt.value(),
                    key + "." + anim_name + ".palette_resolution_strategy",
                    "Animation Config (" + anim_name + ") per-anim strategy",
                    make_source_string(format, file_path, pal_mark),
                    make_source_details(format, file_path, pal_mark)};
            }

            // Parse key_frame_resolution_strategy (optional scalar — per-anim override)
            if (anim_node["key_frame_resolution_strategy"].IsDefined()) {
                const auto &strategy_node = anim_node["key_frame_resolution_strategy"];
                const auto strategy_str = strategy_node.as<std::string>();
                const auto strategy_opt = anim_key_frame_resolution_strategy_from_str(strategy_str);
                if (!strategy_opt.has_value()) {
                    const auto strategy_mark = strategy_node.Mark();
                    const auto strategy_source = make_source_string(format, file_path, strategy_mark);
                    const auto strategy_details = make_source_details(format, file_path, strategy_mark);
                    const auto error = format->format(
                        "'{}' animation '{}' key_frame_resolution_strategy has invalid value '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{anim_name, Style::bold},
                        FormatParam{strategy_str, Style::bold});
                    return LayerValue<PerAnimOverrides>::invalid(error, strategy_source, strategy_details);
                }
                const auto kf_mark = strategy_node.Mark();
                anim_config.key_frame_resolution_strategy = ConfigPODField{
                    strategy_opt.value(),
                    key + "." + anim_name + ".key_frame_resolution_strategy",
                    "Animation Config (" + anim_name + ") key_frame_resolution_strategy",
                    make_source_string(format, file_path, kf_mark),
                    make_source_details(format, file_path, kf_mark)};
            }

            // Parse multi_palette_subtile_resolution_strategy (optional scalar — per-anim override)
            if (anim_node["multi_palette_subtile_resolution_strategy"].IsDefined()) {
                const auto &strategy_node = anim_node["multi_palette_subtile_resolution_strategy"];
                const auto strategy_str = strategy_node.as<std::string>();
                const auto strategy_opt = anim_multi_pal_subtile_resolution_strategy_from_str(strategy_str);
                if (!strategy_opt.has_value()) {
                    const auto strategy_mark = strategy_node.Mark();
                    const auto strategy_source = make_source_string(format, file_path, strategy_mark);
                    const auto strategy_details = make_source_details(format, file_path, strategy_mark);
                    const auto error = format->format(
                        "'{}' animation '{}' multi_palette_subtile_resolution_strategy has invalid value '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{anim_name, Style::bold},
                        FormatParam{strategy_str, Style::bold});
                    return LayerValue<PerAnimOverrides>::invalid(error, strategy_source, strategy_details);
                }
                const auto mps_mark = strategy_node.Mark();
                anim_config.multi_pal_subtile_resolution_strategy = ConfigPODField{
                    strategy_opt.value(),
                    key + "." + anim_name + ".multi_palette_subtile_resolution_strategy",
                    "Animation Config (" + anim_name + ") multi_palette_subtile_resolution_strategy",
                    make_source_string(format, file_path, mps_mark),
                    make_source_details(format, file_path, mps_mark)};
            }

            // Parse per_tile_palette_resolution_strategies (optional sequence — per-tile most specific tier)
            if (anim_node["per_tile_palette_resolution_strategies"].IsDefined()) {
                const auto &strategies_node = anim_node["per_tile_palette_resolution_strategies"];
                if (!strategies_node.IsSequence()) {
                    const auto strategies_mark = strategies_node.Mark();
                    const auto strategies_source = make_source_string(format, file_path, strategies_mark);
                    const auto strategies_details = make_source_details(format, file_path, strategies_mark);
                    const auto error = format->format(
                        "'{}' animation '{}' per_tile_palette_resolution_strategies must be a sequence.",
                        FormatParam{key, Style::bold},
                        FormatParam{anim_name, Style::bold});
                    return LayerValue<PerAnimOverrides>::invalid(error, strategies_source, strategies_details);
                }

                for (std::size_t i = 0; i < strategies_node.size(); ++i) {
                    const auto strategy_str = strategies_node[i].as<std::string>();
                    if (strategy_str == "_") {
                        anim_config.per_tile_pal_resolution_strategies.emplace_back();
                    }
                    else {
                        const auto strategy_opt = anim_pal_resolution_strategy_from_str(strategy_str);
                        if (!strategy_opt.has_value()) {
                            const auto strategy_mark = strategies_node[i].Mark();
                            const auto strategy_source = make_source_string(format, file_path, strategy_mark);
                            const auto strategy_details = make_source_details(format, file_path, strategy_mark);
                            const auto error = format->format(
                                "'{}' animation '{}' per_tile_palette_resolution_strategies[{}] has invalid value "
                                "'{}'.",
                                FormatParam{key, Style::bold},
                                FormatParam{anim_name, Style::bold},
                                FormatParam{i, Style::bold},
                                FormatParam{strategy_str, Style::bold});
                            return LayerValue<PerAnimOverrides>::invalid(error, strategy_source, strategy_details);
                        }
                        const auto tile_mark = strategies_node[i].Mark();
                        anim_config.per_tile_pal_resolution_strategies.push_back(
                            ConfigPODField{
                                strategy_opt.value(),
                                key + "." + anim_name + ".per_tile_palette_resolution_strategies[" + std::to_string(i) +
                                    "]",
                                "Animation Config (" + anim_name + ") subtile " + std::to_string(i),
                                make_source_string(format, file_path, tile_mark),
                                make_source_details(format, file_path, tile_mark)});
                    }
                }
            }

            configs[anim_name] = std::move(anim_config);
        }

        return LayerValue<PerAnimOverrides>::valid(std::move(configs), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as animation configs: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<PerAnimOverrides>::invalid(error, source, details);
    }
}

// Parses a mask scalar written as a string so hexadecimal (0x...), decimal, and octal literals all parse regardless of
// yaml-cpp's numeric handling. Returns nullopt on any parse or 32-bit range failure.
[[nodiscard]] std::optional<std::uint32_t> parse_mask_scalar(const std::string &text)
{
    try {
        std::size_t consumed = 0;
        const unsigned long parsed = std::stoul(text, &consumed, 0);
        if (consumed != text.size() || parsed > 0xFFFFFFFFUL) {
            return std::nullopt;
        }
        return static_cast<std::uint32_t>(parsed);
    }
    catch (const std::exception &) {
        return std::nullopt;
    }
}

// Accepts both the underscore and hyphen spellings of the header-format enum names.
[[nodiscard]] std::optional<HeaderFormat> header_format_from_config_str(const std::string &text)
{
    if (text == "enums_only" || text == "enums-only") {
        return HeaderFormat::enums_only;
    }
    if (text == "defines_only" || text == "defines-only") {
        return HeaderFormat::defines_only;
    }
    if (text == "either") {
        return HeaderFormat::either;
    }
    return std::nullopt;
}

// Returns the first key of a YAML map that is not in the allowed set, or nullopt if all keys are known. Sequence
// children of config values bypass the global unknown-key validator, so field/override entries police their own keys.
[[nodiscard]] std::optional<std::string>
first_unknown_key(const YAML::Node &map_node, const std::unordered_set<std::string> &allowed)
{
    for (const auto &kv : map_node) {
        const auto member = kv.first.as<std::string>();
        if (!allowed.contains(member)) {
            return member;
        }
    }
    return std::nullopt;
}

LayerValue<MetatileAttrFieldSpecs> parse_metatile_attr_fields(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<MetatileAttrFieldSpecs>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsSequence()) {
            return LayerValue<MetatileAttrFieldSpecs>::invalid(
                format->format("'{}' must be a sequence of field definitions.", FormatParam{key, Style::bold}),
                source,
                details);
        }

        const std::unordered_set<std::string> field_keys{"name", "mask", "frlg_mask", "default", "provider"};
        const std::unordered_set<std::string> provider_keys{"header", "prefix", "skipped", "format"};

        MetatileAttrFieldSpecs specs;
        for (std::size_t i = 0; i < node.size(); ++i) {
            const auto &field_node = node[i];
            const auto field_mark = field_node.Mark();
            const auto field_source = make_source_string(format, file_path, field_mark);
            const auto field_details = make_source_details(format, file_path, field_mark);

            if (!field_node.IsMap()) {
                return LayerValue<MetatileAttrFieldSpecs>::invalid(
                    format->format("'{}[{}]' must be a map.", FormatParam{key, Style::bold}, FormatParam{i}),
                    field_source,
                    field_details);
            }
            if (auto unknown = first_unknown_key(field_node, field_keys); unknown.has_value()) {
                return LayerValue<MetatileAttrFieldSpecs>::invalid(
                    format->format(
                        "'{}[{}]' has unknown key '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{i},
                        FormatParam{unknown.value(), Style::bold}),
                    field_source,
                    field_details);
            }

            MetatileAttrFieldSpec spec;
            const auto name_node = field_node["name"];
            if (!name_node.IsDefined()) {
                return LayerValue<MetatileAttrFieldSpecs>::invalid(
                    format->format(
                        "'{}[{}]' is missing required 'name' field.", FormatParam{key, Style::bold}, FormatParam{i}),
                    field_source,
                    field_details);
            }
            spec.name = name_node.as<std::string>();

            for (const auto &[member, target] :
                 std::initializer_list<std::pair<const char *, std::optional<std::uint32_t> *>>{
                     {"mask", &spec.mask}, {"frlg_mask", &spec.frlg_mask}, {"default", &spec.default_value}}) {
                if (field_node[member].IsDefined()) {
                    const auto text = field_node[member].as<std::string>();
                    const auto parsed = parse_mask_scalar(text);
                    if (!parsed.has_value()) {
                        return LayerValue<MetatileAttrFieldSpecs>::invalid(
                            format->format(
                                "'{}[{}].{}' is not a valid 32-bit integer: '{}'.",
                                FormatParam{key, Style::bold},
                                FormatParam{i},
                                FormatParam{member, Style::bold},
                                FormatParam{text, Style::bold}),
                            field_source,
                            field_details);
                    }
                    *target = parsed;
                }
            }

            if (field_node["provider"].IsDefined()) {
                const auto &provider_node = field_node["provider"];
                if (!provider_node.IsMap()) {
                    return LayerValue<MetatileAttrFieldSpecs>::invalid(
                        format->format(
                            "'{}[{}].provider' must be a map.", FormatParam{key, Style::bold}, FormatParam{i}),
                        field_source,
                        field_details);
                }
                if (auto unknown = first_unknown_key(provider_node, provider_keys); unknown.has_value()) {
                    return LayerValue<MetatileAttrFieldSpecs>::invalid(
                        format->format(
                            "'{}[{}].provider' has unknown key '{}'.",
                            FormatParam{key, Style::bold},
                            FormatParam{i},
                            FormatParam{unknown.value(), Style::bold}),
                        field_source,
                        field_details);
                }

                ProviderSpec provider;
                if (!provider_node["header"].IsDefined() || !provider_node["prefix"].IsDefined()) {
                    return LayerValue<MetatileAttrFieldSpecs>::invalid(
                        format->format(
                            "'{}[{}].provider' requires both 'header' and 'prefix'.",
                            FormatParam{key, Style::bold},
                            FormatParam{i}),
                        field_source,
                        field_details);
                }
                provider.header = provider_node["header"].as<std::string>();
                provider.prefix = provider_node["prefix"].as<std::string>();
                if (provider_node["skipped"].IsDefined()) {
                    if (!provider_node["skipped"].IsSequence()) {
                        return LayerValue<MetatileAttrFieldSpecs>::invalid(
                            format->format(
                                "'{}[{}].provider.skipped' must be a sequence.",
                                FormatParam{key, Style::bold},
                                FormatParam{i}),
                            field_source,
                            field_details);
                    }
                    for (std::size_t j = 0; j < provider_node["skipped"].size(); ++j) {
                        provider.skipped.insert(provider_node["skipped"][j].as<std::string>());
                    }
                }
                if (provider_node["format"].IsDefined()) {
                    const auto fmt_str = provider_node["format"].as<std::string>();
                    const auto fmt = header_format_from_config_str(fmt_str);
                    if (!fmt.has_value()) {
                        return LayerValue<MetatileAttrFieldSpecs>::invalid(
                            format->format(
                                "'{}[{}].provider.format' has invalid value '{}'.",
                                FormatParam{key, Style::bold},
                                FormatParam{i},
                                FormatParam{fmt_str, Style::bold}),
                            field_source,
                            field_details);
                    }
                    provider.format = fmt.value();
                }
                spec.provider = std::move(provider);
            }

            specs.push_back(std::move(spec));
        }

        return LayerValue<MetatileAttrFieldSpecs>::valid(std::move(specs), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format(
            "Failed to parse '{}' as metatile attribute fields: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<MetatileAttrFieldSpecs>::invalid(error, source, details);
    }
}

LayerValue<MetatileAttrFieldOverrides> parse_metatile_attr_field_overrides(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<MetatileAttrFieldOverrides>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsMap()) {
            return LayerValue<MetatileAttrFieldOverrides>::invalid(
                format->format("'{}' must be a map of field names to overrides.", FormatParam{key, Style::bold}),
                source,
                details);
        }

        const std::unordered_set<std::string> override_keys{"mask", "frlg_mask", "default", "provider"};
        const std::unordered_set<std::string> provider_keys{"header", "prefix", "skipped", "format"};

        MetatileAttrFieldOverrides overrides;
        for (const auto &kv : node) {
            const auto field_name = kv.first.as<std::string>();
            const auto &override_node = kv.second;
            const auto field_mark = kv.first.Mark();
            const auto field_source = make_source_string(format, file_path, field_mark);
            const auto field_details = make_source_details(format, file_path, field_mark);

            if (!override_node.IsMap()) {
                return LayerValue<MetatileAttrFieldOverrides>::invalid(
                    format->format(
                        "'{}' override for '{}' must be a map.",
                        FormatParam{key, Style::bold},
                        FormatParam{field_name, Style::bold}),
                    field_source,
                    field_details);
            }
            if (auto unknown = first_unknown_key(override_node, override_keys); unknown.has_value()) {
                return LayerValue<MetatileAttrFieldOverrides>::invalid(
                    format->format(
                        "'{}' override for '{}' has unknown key '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{field_name, Style::bold},
                        FormatParam{unknown.value(), Style::bold}),
                    field_source,
                    field_details);
            }

            MetatileAttrFieldOverride override_value;
            for (const auto &[member, target] :
                 std::initializer_list<std::pair<const char *, std::optional<std::uint32_t> *>>{
                     {"mask", &override_value.mask},
                     {"frlg_mask", &override_value.frlg_mask},
                     {"default", &override_value.default_value}}) {
                if (override_node[member].IsDefined()) {
                    const auto text = override_node[member].as<std::string>();
                    const auto parsed = parse_mask_scalar(text);
                    if (!parsed.has_value()) {
                        return LayerValue<MetatileAttrFieldOverrides>::invalid(
                            format->format(
                                "'{}' override for '{}' has invalid '{}': '{}'.",
                                FormatParam{key, Style::bold},
                                FormatParam{field_name, Style::bold},
                                FormatParam{member, Style::bold},
                                FormatParam{text, Style::bold}),
                            field_source,
                            field_details);
                    }
                    *target = parsed;
                }
            }

            if (override_node["provider"].IsDefined()) {
                const auto &provider_node = override_node["provider"];
                ProviderSpecOverride provider_override;
                if (provider_node.IsNull()) {
                    // `provider: null` removes the provider entirely, turning the field into a raw field.
                    provider_override.remove = true;
                }
                else if (provider_node.IsMap()) {
                    if (auto unknown = first_unknown_key(provider_node, provider_keys); unknown.has_value()) {
                        return LayerValue<MetatileAttrFieldOverrides>::invalid(
                            format->format(
                                "'{}' override for '{}' has unknown provider key '{}'.",
                                FormatParam{key, Style::bold},
                                FormatParam{field_name, Style::bold},
                                FormatParam{unknown.value(), Style::bold}),
                            field_source,
                            field_details);
                    }
                    if (provider_node["header"].IsDefined()) {
                        provider_override.header = provider_node["header"].as<std::string>();
                    }
                    if (provider_node["prefix"].IsDefined()) {
                        provider_override.prefix = provider_node["prefix"].as<std::string>();
                    }
                    if (provider_node["skipped"].IsDefined()) {
                        if (!provider_node["skipped"].IsSequence()) {
                            return LayerValue<MetatileAttrFieldOverrides>::invalid(
                                format->format(
                                    "'{}' override for '{}' provider.skipped must be a sequence.",
                                    FormatParam{key, Style::bold},
                                    FormatParam{field_name, Style::bold}),
                                field_source,
                                field_details);
                        }
                        std::unordered_set<std::string> skipped;
                        for (std::size_t j = 0; j < provider_node["skipped"].size(); ++j) {
                            skipped.insert(provider_node["skipped"][j].as<std::string>());
                        }
                        provider_override.skipped = std::move(skipped);
                    }
                    if (provider_node["format"].IsDefined()) {
                        const auto fmt_str = provider_node["format"].as<std::string>();
                        const auto fmt = header_format_from_config_str(fmt_str);
                        if (!fmt.has_value()) {
                            return LayerValue<MetatileAttrFieldOverrides>::invalid(
                                format->format(
                                    "'{}' override for '{}' provider.format has invalid value '{}'.",
                                    FormatParam{key, Style::bold},
                                    FormatParam{field_name, Style::bold},
                                    FormatParam{fmt_str, Style::bold}),
                                field_source,
                                field_details);
                        }
                        provider_override.format = fmt.value();
                    }
                }
                else {
                    return LayerValue<MetatileAttrFieldOverrides>::invalid(
                        format->format(
                            "'{}' override for '{}' provider must be a map or null.",
                            FormatParam{key, Style::bold},
                            FormatParam{field_name, Style::bold}),
                        field_source,
                        field_details);
                }
                override_value.provider = std::move(provider_override);
            }

            overrides[field_name] = std::move(override_value);
        }

        return LayerValue<MetatileAttrFieldOverrides>::valid(std::move(overrides), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format(
            "Failed to parse '{}' as metatile attribute field overrides: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<MetatileAttrFieldOverrides>::invalid(error, source, details);
    }
}

LayerValue<AnimKeyFrameResolutionStrategy> parse_anim_key_frame_resolution_strategy(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<AnimKeyFrameResolutionStrategy>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = anim_key_frame_resolution_strategy_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'.", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<AnimKeyFrameResolutionStrategy>::invalid(error, source, details);
        }

        return LayerValue<AnimKeyFrameResolutionStrategy>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format(
            "Failed to parse '{}' as AnimKeyFrameResolutionStrategy: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<AnimKeyFrameResolutionStrategy>::invalid(error, source, details);
    }
}

LayerValue<AnimMultiPalSubtileResolutionStrategy> parse_anim_multi_pal_subtile_resolution_strategy(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<AnimMultiPalSubtileResolutionStrategy>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = anim_multi_pal_subtile_resolution_strategy_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'.", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<AnimMultiPalSubtileResolutionStrategy>::invalid(error, source, details);
        }

        return LayerValue<AnimMultiPalSubtileResolutionStrategy>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format(
            "Failed to parse '{}' as AnimMultiPalSubtileResolutionStrategy: {}.",
            FormatParam{key, Style::bold},
            e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<AnimMultiPalSubtileResolutionStrategy>::invalid(error, source, details);
    }
}

LayerValue<FrameLinking> parse_frame_linking(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<FrameLinking>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = frame_linking_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'.", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<FrameLinking>::invalid(error, source, details);
        }

        return LayerValue<FrameLinking>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as FrameLinking: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<FrameLinking>::invalid(error, source, details);
    }
}

LayerValue<TileSharingPacking> parse_tile_sharing_packing(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<TileSharingPacking>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = tile_sharing_packing_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'.", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<TileSharingPacking>::invalid(error, source, details);
        }

        return LayerValue<TileSharingPacking>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as TileSharingPacking: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<TileSharingPacking>::invalid(error, source, details);
    }
}

LayerValue<TileSharingAlignment> parse_tile_sharing_alignment(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<TileSharingAlignment>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = tile_sharing_alignment_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'.", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<TileSharingAlignment>::invalid(error, source, details);
        }

        return LayerValue<TileSharingAlignment>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format(
            "Failed to parse '{}' as TileSharingAlignment: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<TileSharingAlignment>::invalid(error, source, details);
    }
}

LayerValue<PackingStrategyType> parse_packing_strategy_type(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<PackingStrategyType>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = packing_strategy_type_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'.", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<PackingStrategyType>::invalid(error, source, details);
        }

        return LayerValue<PackingStrategyType>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as PackingStrategyType: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<PackingStrategyType>::invalid(error, source, details);
    }
}

LayerValue<PrimaryPairingMode> parse_primary_pairing_mode(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<PrimaryPairingMode>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        const auto node_value = node.as<std::string>();
        const auto mode_opt = primary_pairing_mode_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'.", FormatParam{key, Style::bold}, FormatParam{node_value, Style::bold});
            return LayerValue<PrimaryPairingMode>::invalid(error, source, details);
        }

        return LayerValue<PrimaryPairingMode>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("Failed to parse '{}' as PrimaryPairingMode: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<PrimaryPairingMode>::invalid(error, source, details);
    }
}

LayerValue<FrlgAlternateMaskMode> parse_frlg_alternate_mask_mode(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<FrlgAlternateMaskMode>::not_provided();
    }

    const auto mark = node.Mark();
    const auto source = make_source_string(format, file_path, mark);
    const auto details = make_source_details(format, file_path, mark);

    // Accept a YAML boolean as an alias: true maps to always, false maps to never. yaml-cpp throws
    // a YAML::Exception for a non-boolean scalar, in which case we fall back to fuzzy string parsing.
    try {
        const auto bool_value = node.as<bool>();
        return LayerValue<FrlgAlternateMaskMode>::valid(
            bool_value ? FrlgAlternateMaskMode::always : FrlgAlternateMaskMode::never, key, source, details);
    }
    catch (const YAML::Exception &) {
        // Not a boolean scalar; fall through to string parsing below.
    }

    try {
        const auto node_value = node.as<std::string>();
        const auto mode_opt = frlg_alternate_mask_mode_from_str(node_value);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'. Valid values are true, false, automatic, always, or never.",
                FormatParam{key, Style::bold},
                FormatParam{node_value, Style::bold});
            return LayerValue<FrlgAlternateMaskMode>::invalid(error, source, details);
        }

        return LayerValue<FrlgAlternateMaskMode>::valid(mode_opt.value(), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto error = format->format(
            "Failed to parse '{}' as FrlgAlternateMaskMode: {}.", FormatParam{key, Style::bold}, e.what());
        return LayerValue<FrlgAlternateMaskMode>::invalid(error, source, details);
    }
}

// Parses an optional layer-type mask written as a scalar. A parsed value (including 0, which disables the layer type)
// yields a present optional; an absent node yields not_provided so the inference provider and size convention can
// supply it. The scalar is parsed as a string so hex/decimal/octal literals all work regardless of yaml-cpp's numeric
// handling.
LayerValue<std::optional<std::uint32_t>> parse_layer_type_mask(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<std::optional<std::uint32_t>>::not_provided();
    }

    const auto mark = node.Mark();
    const auto source = make_source_string(format, file_path, mark);
    const auto details = make_source_details(format, file_path, mark);

    try {
        const auto text = node.as<std::string>();
        const auto parsed = parse_mask_scalar(text);
        if (!parsed.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}'. Expected a 32-bit integer mask (for example 0xF000); use 0 to disable "
                "the layer type.",
                FormatParam{key, Style::bold},
                FormatParam{text, Style::bold});
            return LayerValue<std::optional<std::uint32_t>>::invalid(error, source, details);
        }
        return LayerValue<std::optional<std::uint32_t>>::valid(
            std::optional<std::uint32_t>{parsed.value()}, key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto error =
            format->format("Failed to parse '{}' as a layer type mask: {}.", FormatParam{key, Style::bold}, e.what());
        return LayerValue<std::optional<std::uint32_t>>::invalid(error, source, details);
    }
}

LayerValue<PackingStrategyParams> parse_packing_strategy_params(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<PackingStrategyParams>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsMap()) {
            const auto error = format->format(
                "'{}' must be a map of strategy names to parameter objects.", FormatParam{key, Style::bold});
            return LayerValue<PackingStrategyParams>::invalid(error, source, details);
        }

        PackingStrategyParams params;

        // Parse backtracking sub-map
        if (node["backtracking"].IsDefined()) {
            const auto &bt_node = node["backtracking"];
            if (!bt_node.IsMap()) {
                const auto bt_mark = bt_node.Mark();
                const auto bt_source = make_source_string(format, file_path, bt_mark);
                const auto bt_details = make_source_details(format, file_path, bt_mark);
                const auto error = format->format("'{}' backtracking must be a map.", FormatParam{key, Style::bold});
                return LayerValue<PackingStrategyParams>::invalid(error, bt_source, bt_details);
            }

            if (bt_node["search_algorithm"].IsDefined()) {
                const auto &sa_node = bt_node["search_algorithm"];
                const auto sa_str = sa_node.as<std::string>();
                const auto sa_opt = search_algorithm_from_str(sa_str);
                if (!sa_opt.has_value()) {
                    const auto sa_mark = sa_node.Mark();
                    const auto sa_source = make_source_string(format, file_path, sa_mark);
                    const auto sa_details = make_source_details(format, file_path, sa_mark);
                    const auto error = format->format(
                        "'{}' backtracking search_algorithm has invalid value '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{sa_str, Style::bold});
                    return LayerValue<PackingStrategyParams>::invalid(error, sa_source, sa_details);
                }
                const auto sa_mark = sa_node.Mark();
                params.backtracking.search_algorithm = ConfigPODField{
                    sa_opt.value(),
                    key + ".backtracking.search_algorithm",
                    "Packing Strategy Params (backtracking) search_algorithm",
                    make_source_string(format, file_path, sa_mark),
                    make_source_details(format, file_path, sa_mark)};
            }

            if (bt_node["node_cutoff"].IsDefined()) {
                const auto &nc_node = bt_node["node_cutoff"];
                const auto nc_val = nc_node.as<std::size_t>();
                const auto nc_mark = nc_node.Mark();
                params.backtracking.node_cutoff = ConfigPODField{
                    nc_val,
                    key + ".backtracking.node_cutoff",
                    "Packing Strategy Params (backtracking) node_cutoff",
                    make_source_string(format, file_path, nc_mark),
                    make_source_details(format, file_path, nc_mark)};
            }

            if (bt_node["best_branches"].IsDefined()) {
                const auto &bb_node = bt_node["best_branches"];
                const auto bb_val = bb_node.as<std::size_t>();
                const auto bb_mark = bb_node.Mark();
                params.backtracking.best_branches = ConfigPODField{
                    bb_val,
                    key + ".backtracking.best_branches",
                    "Packing Strategy Params (backtracking) best_branches",
                    make_source_string(format, file_path, bb_mark),
                    make_source_details(format, file_path, bb_mark)};
            }

            if (bt_node["smart_prune"].IsDefined()) {
                const auto &sp_node = bt_node["smart_prune"];
                const auto sp_val = sp_node.as<bool>();
                const auto sp_mark = sp_node.Mark();
                params.backtracking.smart_prune = ConfigPODField{
                    sp_val,
                    key + ".backtracking.smart_prune",
                    "Packing Strategy Params (backtracking) smart_prune",
                    make_source_string(format, file_path, sp_mark),
                    make_source_details(format, file_path, sp_mark)};
            }
        }

        // Parse overload_and_remove sub-map
        if (node["overload_and_remove"].IsDefined()) {
            const auto &oar_node = node["overload_and_remove"];
            if (!oar_node.IsMap()) {
                const auto oar_mark = oar_node.Mark();
                const auto oar_source = make_source_string(format, file_path, oar_mark);
                const auto oar_details = make_source_details(format, file_path, oar_mark);
                const auto error =
                    format->format("'{}' overload_and_remove must be a map.", FormatParam{key, Style::bold});
                return LayerValue<PackingStrategyParams>::invalid(error, oar_source, oar_details);
            }

            if (oar_node["max_attempts"].IsDefined()) {
                const auto &ma_node = oar_node["max_attempts"];
                const auto ma_val = ma_node.as<std::size_t>();
                const auto ma_mark = ma_node.Mark();
                params.overload_and_remove.max_attempts = ConfigPODField{
                    ma_val,
                    key + ".overload_and_remove.max_attempts",
                    "Packing Strategy Params (overload_and_remove) max_attempts",
                    make_source_string(format, file_path, ma_mark),
                    make_source_details(format, file_path, ma_mark)};
            }

            if (oar_node["seed"].IsDefined()) {
                const auto &seed_node = oar_node["seed"];
                const auto seed_val = seed_node.as<std::uint64_t>();
                const auto seed_mark = seed_node.Mark();
                params.overload_and_remove.seed = ConfigPODField{
                    seed_val,
                    key + ".overload_and_remove.seed",
                    "Packing Strategy Params (overload_and_remove) seed",
                    make_source_string(format, file_path, seed_mark),
                    make_source_details(format, file_path, seed_mark)};
            }

            if (oar_node["shuffle_strategy"].IsDefined()) {
                const auto &ss_node = oar_node["shuffle_strategy"];
                const auto ss_str = ss_node.as<std::string>();
                const auto ss_opt = shuffle_strategy_from_str(ss_str);
                if (!ss_opt.has_value()) {
                    const auto ss_mark = ss_node.Mark();
                    const auto ss_source = make_source_string(format, file_path, ss_mark);
                    const auto ss_details = make_source_details(format, file_path, ss_mark);
                    const auto error = format->format(
                        "'{}' overload_and_remove shuffle_strategy has invalid value '{}'.",
                        FormatParam{key, Style::bold},
                        FormatParam{ss_str, Style::bold});
                    return LayerValue<PackingStrategyParams>::invalid(error, ss_source, ss_details);
                }
                const auto ss_mark = ss_node.Mark();
                params.overload_and_remove.shuffle_strategy = ConfigPODField{
                    ss_opt.value(),
                    key + ".overload_and_remove.shuffle_strategy",
                    "Packing Strategy Params (overload_and_remove) shuffle_strategy",
                    make_source_string(format, file_path, ss_mark),
                    make_source_details(format, file_path, ss_mark)};
            }
        }

        return LayerValue<PackingStrategyParams>::valid(std::move(params), key, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format(
            "Failed to parse '{}' as packing strategy params: {}.", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<PackingStrategyParams>::invalid(error, source, details);
    }
}

/**
 * @brief Attempts to load a YAML file and add it to the cache.
 *
 * @details
 * If the file exists and can be parsed, it is added to the cache and returned. If diagnostics is provided, validates
 * the YAML paths and emits errors for unknown keys. If the file doesn't exist or cannot be parsed, returns
 * std::nullopt. Uses a static cache shared across all YamlFileProvider instances.
 *
 * @param path The path to the YAML file to load
 * @param format Text formatter for styled output (used for validation)
 * @param diagnostics User diagnostics for emitting errors (may be nullptr)
 * @param out_had_unknown_keys Optional output flag set to @c true if unknown keys were found during validation.
 * @return The loaded YAML node, or std::nullopt if the file doesn't exist or cannot be parsed
 */
std::optional<YAML::Node> load_yaml_file(
    const std::filesystem::path &path,
    const TextFormatter *format = nullptr,
    const UserDiagnostics *diagnostics = nullptr,
    bool *out_had_unknown_keys = nullptr)
{
    // Check cache first
    const auto cache_it = yaml_cache.find(path);
    if (cache_it != yaml_cache.end()) {
        return cache_it->second;
    }

    // File doesn't exist, return nullopt
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    // Try to load and cache the file
    try {
        auto node = YAML::LoadFile(path.string());
        yaml_cache[path] = node;

        // Also cache the file contents line-by-line for source info
        std::ifstream file{path};
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file_lines_cache[path] = std::move(lines);

        // Validate paths if diagnostics is provided
        if (format != nullptr && diagnostics != nullptr) {
            if (validate_yaml_paths(format, diagnostics, path, node) && out_had_unknown_keys != nullptr) {
                *out_had_unknown_keys = true;
            }
        }

        return node;
    }
    catch (const YAML::Exception &) {
        // Failed to parse YAML, return nullopt
        return std::nullopt;
    }
}

/**
 * @brief Gets the priority-ordered list of config file paths for a given tileset.
 *
 * @details
 * Returns config file paths in priority order (highest to lowest):
 * 1. porytiles/tilesets/{tileset_name}/config.local.yaml (tileset-specific local overrides)
 * 2. porytiles/tilesets/{tileset_name}/config.yaml (tileset-specific config)
 * 3. porytiles/config.local.yaml (project-wide local overrides)
 * 4. porytiles/config.yaml (project-wide defaults)
 *
 * Config files are stored in the centralized Porytiles utility directory rather than
 * within individual tileset folders.
 *
 * @param project_root The project root directory
 * @param tileset The name of the tileset
 * @return Vector of config file paths in priority order
 */
std::vector<std::filesystem::path>
get_tileset_config_path_chain(const std::filesystem::path &project_root, const std::string &tileset)
{
    std::vector<std::filesystem::path> paths;

    // Porytiles utility directory root
    const auto porytiles_dir = project_root / "porytiles";

    // Priority order (highest to lowest):
    // 1. porytiles/tilesets/{tileset_name}/config.local.yaml
    paths.push_back(porytiles_dir / "tilesets" / tileset / "config.local.yaml");

    // 2. porytiles/tilesets/{tileset_name}/config.yaml
    paths.push_back(porytiles_dir / "tilesets" / tileset / "config.yaml");

    // 3. porytiles/config.local.yaml
    paths.push_back(porytiles_dir / "config.local.yaml");

    // 4. porytiles/config.yaml
    paths.push_back(porytiles_dir / "config.yaml");

    return paths;
}

/**
 * @brief Gets the config file path chain for a specific scope in priority order.
 *
 * @details
 * Returns a vector of config file paths that should be searched for config values in priority order from highest to
 * lowest. Dispatches to the appropriate path chain function based on the ConfigScopeType.
 *
 * @param project_root The root directory of the project
 * @param type The configuration scope type (tileset or layout)
 * @param scope The scope name (tileset name or layout name)
 * @return ChainableResult containing vector of config file paths in priority order
 */
ChainableResult<std::vector<std::filesystem::path>>
get_config_path_chain(const std::filesystem::path &project_root, ConfigScopeType type, const std::string &scope)
{
    switch (type) {
    case ConfigScopeType::tileset:
        return get_tileset_config_path_chain(project_root, scope);
    case ConfigScopeType::layout:
        panic("Layout config path chain resolution is not yet implemented.");
    }
    // Should never reach here
    panic("Invalid ConfigScopeType");
}

/**
 * @brief Eagerly loads all YAML config files for a given scope and validates them for unknown keys.
 *
 * @details
 * Forces loading and validation of all YAML config files in the priority chain for the given scope.
 * If any files contain unknown configuration keys, errors are emitted via the diagnostics interface
 * and this function returns @c true to indicate the caller should terminate.
 *
 * @param format Text formatter for styled output
 * @param diagnostics User diagnostics for emitting errors
 * @param project_root The root directory of the project
 * @param type The configuration scope type
 * @param scope The scope name (e.g., tileset name)
 * @return @c true if unknown keys were found (caller should terminate), @c false otherwise.
 */
[[nodiscard]] bool preload_and_validate_yaml_files(
    const TextFormatter *format,
    const UserDiagnostics *diagnostics,
    const std::filesystem::path &project_root,
    ConfigScopeType type,
    const std::string &scope)
{
    auto paths_result = get_config_path_chain(project_root, type, scope);
    if (!paths_result.has_value()) {
        return false;
    }

    bool had_unknown_keys = false;
    for (const auto &path : paths_result.value()) {
        load_yaml_file(path, format, diagnostics, &had_unknown_keys);
    }

    return had_unknown_keys;
}

/**
 * @brief Helper to search for a config value across multiple YAML files in priority order.
 *
 * @details
 * Searches through the provided config file paths in priority order. For each file:
 * - Attempts to load the file (using the load function)
 * - Extracts the node at the specified YAML path
 * - Parses the value using the provided parser function
 * - Returns the first valid value found
 * - Returns an error immediately if parsing fails
 * - Continues to the next file if not_provided
 *
 * @tparam T The type of value to return
 * @tparam LoadFunc Function type for loading YAML files (path -> optional<YAML::Node>)
 * @tparam NodeExtractFunc Function type for extracting node (YAML::Node -> YAML::Node)
 * @tparam ParseFunc Function type for parsing value (format, node, key, path -> LayerValue<T>)
 * @param format The text formatter to use
 * @param paths Config file paths to search in priority order
 * @param load_func Function to load a YAML file
 * @param extract_node_func Function to extract the target node from the YAML doc
 * @param parse_func Function to parse the value from the node
 * @param key The configuration key name (for error messages)
 * @param provider_name The provider-specific name for this value
 * @return The first valid LayerValue found, or not_provided if not found in any file
 */
template <typename T, typename LoadFunc, typename NodeExtractFunc, typename ParseFunc>
LayerValue<T> search_config_files(
    const TextFormatter *format,
    const std::vector<std::filesystem::path> &paths,
    LoadFunc load_func,
    NodeExtractFunc extract_node_func,
    ParseFunc parse_func,
    const std::string &key,
    const std::string &provider_name)
{
    for (const auto &path : paths) {
        const auto yaml_doc = load_func(path);
        if (!yaml_doc.has_value()) {
            // File doesn't exist or couldn't be loaded, try next file
            continue;
        }

        try {
            const auto node = extract_node_func(yaml_doc.value());
            auto result = parse_func(format, node, key, path.string());

            // If we got a valid value or an error, return it immediately
            if (result.state == ValidationState::valid || result.state == ValidationState::invalid) {
                result.source_key = provider_name;
                return result;
            }

            // If not_provided, continue to next file
        }
        catch (const YAML::Exception &) {
            // Node extraction or parsing threw an exception, treat as not_provided for this file
            // and continue to the next file
            continue;
        }
    }

    // Not found in any file
    return LayerValue<T>::not_provided();
}

} // namespace
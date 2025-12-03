#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include "yaml-cpp/yaml.h"

#include "porytiles2/domain/config/patch_mode.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/config/config_provider.hpp"
#include "porytiles2/infra/config/valid_yaml_paths.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

// The anonymous namespace ensures internal linkage per translation unit
// This file is intentionally included only in yaml_file_provider.cpp
namespace {

using namespace porytiles2;

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
 * @param window_size The total number of lines to show in the contextual view (default: 5)
 * @return Vector of formatted strings showing the contextual view
 */
std::vector<std::string> make_source_details(
    const TextFormatter *format, const std::string &file_path, const YAML::Mark &mark, std::size_t window_size = 7)
{
    std::vector<std::string> details;

    const std::filesystem::path path{file_path};
    const auto it = file_lines_cache.find(path);
    if (it == file_lines_cache.end()) {
        return details;
    }

    const auto &lines = it->second;
    const std::size_t line_num = mark.line; // 0-indexed

    if (lines.empty() || line_num >= lines.size()) {
        return details;
    }

    // Calculate window boundaries
    const std::size_t half_window = (window_size - 1) / 2;
    const std::size_t start = (line_num >= half_window) ? line_num - half_window : 0;
    const std::size_t end = std::min(line_num + half_window + 1, lines.size());

    // Build contextual view
    for (std::size_t i = start; i < end; ++i) {
        std::string prefix;
        // Value here is minus-1 because we're zero-indexing line number, but then adding one to display
        if (i < 9) {
            const auto formatted_arrow =
                format->format("{}", FormatParam{"➞     ", Style::bold | Style::italic | Style::yellow});
            prefix = (i == line_num) ? formatted_arrow : "      ";
        }
        else if (i < 99) {
            const auto formatted_arrow =
                format->format("{}", FormatParam{"➞    ", Style::bold | Style::italic | Style::yellow});
            prefix = (i == line_num) ? formatted_arrow : "     ";
        }
        else if (i < 999) {
            const auto formatted_arrow =
                format->format("{}", FormatParam{"➞   ", Style::bold | Style::italic | Style::yellow});
            prefix = (i == line_num) ? formatted_arrow : "    ";
        }
        else if (i < 9999) {
            const auto formatted_arrow =
                format->format("{}", FormatParam{"➞  ", Style::bold | Style::italic | Style::yellow});
            prefix = (i == line_num) ? formatted_arrow : "   ";
        }
        else {
            panic("10000+ line yaml files not supported, file a bug report");
        }

        if (i == line_num) {
            const auto highlight_line =
                format->format("{}", FormatParam{lines[i], Style::bold | Style::italic | Style::yellow});
            details.push_back(prefix + std::to_string(i + 1) + ":   " + highlight_line);
        }
        else {
            details.push_back(prefix + std::to_string(i + 1) + ":   " + lines[i]);
        }
    }

    return details;
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
 * defined in valid_yaml_paths.hpp. For any unknown paths, emits a warning via the
 * UserDiagnostics interface with source location and context.
 *
 * @param format Text formatter for styled output
 * @param diagnostics User diagnostics for emitting warnings (may be nullptr)
 * @param file_path Path to the YAML file being validated
 * @param node The root YAML node to validate
 */
void validate_yaml_paths(
    const TextFormatter *format,
    const UserDiagnostics *diagnostics,
    const std::filesystem::path &file_path,
    const YAML::Node &node)
{
    if (diagnostics == nullptr) {
        return;
    }

    std::vector<std::pair<std::string, YAML::Mark>> paths;
    collect_yaml_paths(node, "", paths);

    for (const auto &[path, mark] : paths) {
        if (valid_yaml_paths.find(path) == valid_yaml_paths.end()) {
            const auto source = make_source_string(format, file_path.string(), mark);
            auto details = make_source_details(format, file_path.string(), mark);

            std::vector<std::string> warning_lines;
            warning_lines.push_back(format->format("unknown configuration key '{}'", FormatParam{path, Style::bold}));
            warning_lines.emplace_back();
            warning_lines.push_back(format->format("Source: {}", FormatParam{source}));
            warning_lines.emplace_back();
            for (auto &detail : details) {
                warning_lines.push_back(std::move(detail));
            }

            diagnostics->warn("unknown-config-key", warning_lines);

            // TODO: add a "did you mean" message that uses levenshtein distance to find closest match
        }
    }
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
        return LayerValue<std::size_t>::valid(value, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("failed to parse '{}' as std::size_t: {}", FormatParam{key, Style::bold}, e.what());
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
        return LayerValue<bool>::valid(value, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = format->format("failed to parse '{}' as bool: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<bool>::invalid(error, source, details);
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
        const auto details = make_source_details(format, file_path, mark);

        if (!node.IsSequence()) {
            const auto error =
                format->format("'{}' must be a sequence [r, g, b] or [r, g, b, a]", FormatParam{key, Style::bold});
            const auto source = make_source_string(format, file_path, mark);
            return LayerValue<Rgba32>::invalid(error, source, details);
        }

        if (node.size() < 3 || node.size() > 4) {
            const auto error = format->format(
                "'{}' must have 3 or 4 elements [r, g, b] or [r, g, b, a], got {}",
                FormatParam{key, Style::bold},
                FormatParam{node.size(), Style::bold});
            const auto source = make_source_string(format, file_path, mark);
            return LayerValue<Rgba32>::invalid(error, source, details);
        }

        const auto r = node[0].as<std::uint8_t>();
        const auto g = node[1].as<std::uint8_t>();
        const auto b = node[2].as<std::uint8_t>();
        const auto a = (node.size() == 4) ? node[3].as<std::uint8_t>() : Rgba32::alpha_opaque;

        const Rgba32 color{r, g, b, a};
        const auto source = make_source_string(format, file_path, mark);
        return LayerValue<Rgba32>::valid(color, source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("failed to parse '{}' as Rgba32: {}", FormatParam{key, Style::bold}, e.what());
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

                if (!color_node.IsSequence() || color_node.size() < 3 || color_node.size() > 4) {
                    const auto color_mark = color_node.Mark();
                    const auto error = format->format(
                        "'{}[{}].colors[{}]' must be [r, g, b] or [r, g, b, a]",
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

            hints.emplace_back(name, Palette<Rgba32>{std::move(colors)});
        }

        const auto source = make_source_string(format, file_path, mark);
        return LayerValue<std::vector<PaletteHint>>::valid(std::move(hints), source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("failed to parse '{}' as palette hints: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<std::vector<PaletteHint>>::invalid(error, source, details);
    }
}

/**
 * @brief Attempts to parse a TilesPalMode value from a YAML node.
 *
 * @details
 * Expects a string value that matches one of the valid TilesPalMode strings: "true-color" or "greyscale".
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
        const auto details = make_source_details(format, file_path, mark);
        const auto str = node.as<std::string>();
        const auto mode_opt = tiles_pal_mode_from_str(str);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}', expected 'true-color' or 'greyscale'",
                FormatParam{key, Style::bold},
                FormatParam{str, Style::bold});
            const auto source = make_source_string(format, file_path, mark);
            return LayerValue<TilesPalMode>::invalid(error, source, details);
        }

        const auto source = make_source_string(format, file_path, mark);
        return LayerValue<TilesPalMode>::valid(mode_opt.value(), source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("failed to parse '{}' as TilesPalMode: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<TilesPalMode>::invalid(error, source, details);
    }
}

LayerValue<PatchTilesMode> parse_patch_tiles_mode(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<PatchTilesMode>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto details = make_source_details(format, file_path, mark);
        const auto str = node.as<std::string>();
        const auto mode_opt = patch_tiles_mode_from_str(str);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}', expected 'fixed' or 'free'",
                FormatParam{key, Style::bold},
                FormatParam{str, Style::bold});
            const auto source = make_source_string(format, file_path, mark);
            return LayerValue<PatchTilesMode>::invalid(error, source, details);
        }

        const auto source = make_source_string(format, file_path, mark);
        return LayerValue<PatchTilesMode>::valid(mode_opt.value(), source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("failed to parse '{}' as PatchTilesMode: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<PatchTilesMode>::invalid(error, source, details);
    }
}

LayerValue<PatchPalMode> parse_patch_pal_mode(
    const TextFormatter *format, const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return LayerValue<PatchPalMode>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto details = make_source_details(format, file_path, mark);
        const auto str = node.as<std::string>();
        const auto mode_opt = patch_pal_mode_from_str(str);

        if (!mode_opt.has_value()) {
            const auto error = format->format(
                "'{}' has invalid value '{}', expected 'fixed' or 'free'",
                FormatParam{key, Style::bold},
                FormatParam{str, Style::bold});
            const auto source = make_source_string(format, file_path, mark);
            return LayerValue<PatchPalMode>::invalid(error, source, details);
        }

        const auto source = make_source_string(format, file_path, mark);
        return LayerValue<PatchPalMode>::valid(mode_opt.value(), source, details);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error =
            format->format("failed to parse '{}' as PatchPalMode: {}", FormatParam{key, Style::bold}, e.what());
        const auto source = make_source_string(format, file_path, mark);
        const auto details = make_source_details(format, file_path, mark);
        return LayerValue<PatchPalMode>::invalid(error, source, details);
    }
}

/**
 * @brief Attempts to load a YAML file and add it to the cache.
 *
 * @details
 * If the file exists and can be parsed, it is added to the cache and returned. If diagnostics is provided, validates
 * the YAML paths and emits warnings for unknown keys. If the file doesn't exist or cannot be parsed, returns
 * std::nullopt. Uses a static cache shared across all YamlFileProvider instances.
 *
 * @param path The path to the YAML file to load
 * @param format Text formatter for styled output (used for validation)
 * @param diagnostics User diagnostics for emitting warnings (may be nullptr)
 * @return The loaded YAML node, or std::nullopt if the file doesn't exist or cannot be parsed
 */
std::optional<YAML::Node> load_yaml_file(
    const std::filesystem::path &path,
    const TextFormatter *format = nullptr,
    const UserDiagnostics *diagnostics = nullptr)
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
            validate_yaml_paths(format, diagnostics, path, node);
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
 * 1. tileset_folder/config.local.yaml
 * 2. tileset_folder/config.yaml
 * 3. project_root/config.local.yaml
 * 4. project_root/config.yaml
 *
 * @param project_root The project root directory
 * @param key_provider Provider for generating tileset artifact keys and paths
 * @param tileset The name of the tileset
 * @return Vector of config file paths in priority order
 */
std::vector<std::filesystem::path> get_tileset_config_path_chain(
    const std::filesystem::path &project_root,
    const TilesetArtifactKeyProvider *key_provider,
    const std::string &tileset)
{
    /*
     * TODO: once we add layout support, we'll need a separate method to resolve the config path chain for layouts,
     * since layouts will have their own LayoutArtifactKeyProvider.
     */

    std::vector<std::filesystem::path> paths;

    // Get tileset-specific config paths using the key provider
    const auto tileset_local_config_key = key_provider->key_for_local_config(tileset);
    const auto tileset_config_key = key_provider->key_for_config(tileset);

    // Priority order (highest to lowest):
    // 1. tileset_folder/porytiles.local.yaml
    paths.emplace_back(tileset_local_config_key.key());

    // 2. tileset_folder/porytiles.yaml
    paths.emplace_back(tileset_config_key.key());

    // 3. project_root/porytiles.local.yaml
    paths.push_back(project_root / "porytiles.local.yaml");

    // 4. project_root/porytiles.yaml
    paths.push_back(project_root / "porytiles.yaml");

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
 * @param key_provider Provider for generating tileset artifact keys and paths
 * @param type The configuration scope type (tileset or layout)
 * @param scope The scope name (tileset name or layout name)
 * @return Vector of config file paths in priority order
 */
std::vector<std::filesystem::path> get_config_path_chain(
    const std::filesystem::path &project_root,
    const TilesetArtifactKeyProvider *key_provider,
    ConfigScopeType type,
    const std::string &scope)
{
    switch (type) {
    case ConfigScopeType::tileset:
        return get_tileset_config_path_chain(project_root, key_provider, scope);
    case ConfigScopeType::layout:
        panic("TODO: implement layout config path chain resolution");
    }
    // Should never reach here
    panic("Invalid ConfigScopeType");
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
 * @return The first valid LayerValue found, or not_provided if not found in any file
 */
template <typename T, typename LoadFunc, typename NodeExtractFunc, typename ParseFunc>
LayerValue<T> search_config_files(
    const TextFormatter *format,
    const std::vector<std::filesystem::path> &paths,
    LoadFunc load_func,
    NodeExtractFunc extract_node_func,
    ParseFunc parse_func,
    const std::string &key)
{
    for (const auto &path : paths) {
        const auto yaml_doc = load_func(path);
        if (!yaml_doc.has_value()) {
            // File doesn't exist or couldn't be loaded, try next file
            continue;
        }

        try {
            const auto node = extract_node_func(yaml_doc.value());
            const auto result = parse_func(format, node, key, path.string());

            // If we got a valid value or an error, return it immediately
            if (result.state == ValidationState::valid || result.state == ValidationState::invalid) {
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
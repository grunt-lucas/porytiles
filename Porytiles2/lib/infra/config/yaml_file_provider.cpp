#include "porytiles2/infra/config/yaml_file_provider.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <sstream>

#include "fmt/format.h"
#include "yaml-cpp/yaml.h"

#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"

namespace {

/**
 * @brief Attempts to parse a std::size_t value from a YAML node.
 *
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
porytiles2::LayerValue<std::size_t>
parse_size_t(const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return porytiles2::LayerValue<std::size_t>::not_provided();
    }

    try {
        const auto value = node.as<std::size_t>();
        const auto mark = node.Mark();
        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<std::size_t>::valid(value, source);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = fmt::format("Failed to parse '{}' as std::size_t: {}", key, e.what());
        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<std::size_t>::invalid(error, source);
    }
}

/**
 * @brief Attempts to parse a bool value from a YAML node.
 *
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
porytiles2::LayerValue<bool> parse_bool(const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return porytiles2::LayerValue<bool>::not_provided();
    }

    try {
        const auto value = node.as<bool>();
        const auto mark = node.Mark();
        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<bool>::valid(value, source);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = fmt::format("Failed to parse '{}' as bool: {}", key, e.what());
        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<bool>::invalid(error, source);
    }
}

/**
 * @brief Attempts to parse an Rgba32 color from a YAML node.
 *
 * @details
 * Expects the YAML node to be a sequence with 3 or 4 elements: [r, g, b] or [r, g, b, a]. If alpha is not provided, it
 * defaults to 255 (opaque).
 *
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
porytiles2::LayerValue<porytiles2::Rgba32>
parse_rgba32(const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return porytiles2::LayerValue<porytiles2::Rgba32>::not_provided();
    }

    try {
        const auto mark = node.Mark();

        if (!node.IsSequence()) {
            const auto error = fmt::format("'{}' must be a sequence [r, g, b] or [r, g, b, a]", key);
            const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
            return porytiles2::LayerValue<porytiles2::Rgba32>::invalid(error, source);
        }

        if (node.size() < 3 || node.size() > 4) {
            const auto error =
                fmt::format("'{}' must have 3 or 4 elements [r, g, b] or [r, g, b, a], got {}", key, node.size());
            const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
            return porytiles2::LayerValue<porytiles2::Rgba32>::invalid(error, source);
        }

        const auto r = node[0].as<std::uint8_t>();
        const auto g = node[1].as<std::uint8_t>();
        const auto b = node[2].as<std::uint8_t>();
        const auto a = (node.size() == 4) ? node[3].as<std::uint8_t>() : porytiles2::Rgba32::alpha_opaque;

        const porytiles2::Rgba32 color{r, g, b, a};
        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<porytiles2::Rgba32>::valid(color, source);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = fmt::format("Failed to parse '{}' as Rgba32: {}", key, e.what());
        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<porytiles2::Rgba32>::invalid(error, source);
    }
}

/**
 * @brief Attempts to parse a TilesPalMode value from a YAML node.
 *
 * @details
 * Expects a string value that matches one of the valid TilesPalMode strings: "true-color" or "greyscale".
 *
 * @param node The YAML node to parse
 * @param key The configuration key name (for error messages)
 * @param file_path The YAML file path (for source info)
 * @return LayerValue containing the parsed value, error, or not_provided status
 */
porytiles2::LayerValue<porytiles2::TilesPalMode>
parse_tiles_pal_mode(const YAML::Node &node, const std::string &key, const std::string &file_path)
{
    if (!node.IsDefined()) {
        return porytiles2::LayerValue<porytiles2::TilesPalMode>::not_provided();
    }

    try {
        const auto mark = node.Mark();
        const auto str = node.as<std::string>();
        const auto mode_opt = porytiles2::tiles_pal_mode_from_str(str);

        if (!mode_opt.has_value()) {
            const auto error =
                fmt::format("'{}' has invalid value '{}', expected 'true-color' or 'greyscale'", key, str);
            const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
            return porytiles2::LayerValue<porytiles2::TilesPalMode>::invalid(error, source);
        }

        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<porytiles2::TilesPalMode>::valid(mode_opt.value(), source);
    }
    catch (const YAML::Exception &e) {
        const auto mark = node.Mark();
        const auto error = fmt::format("Failed to parse '{}' as TilesPalMode: {}", key, e.what());
        const auto source = fmt::format("{}:{}", file_path, mark.line + 1);
        return porytiles2::LayerValue<porytiles2::TilesPalMode>::invalid(error, source);
    }
}

/**
 * @brief Attempts to load a YAML file and add it to the cache.
 *
 * @details
 * If the file exists and can be parsed, it is added to the cache and returned. If the file doesn't exist or cannot be
 * parsed, returns std::nullopt. Uses a static cache shared across all YamlFileProvider instances.
 *
 * @param path The path to the YAML file to load
 * @return The loaded YAML node, or std::nullopt if the file doesn't exist or cannot be parsed
 */
std::optional<YAML::Node> load_yaml_file(const std::filesystem::path &path)
{
    // Static cache shared across all YamlFileProvider instances
    static std::map<std::filesystem::path, YAML::Node> yaml_cache;

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
std::vector<std::filesystem::path> get_config_paths(
    const std::filesystem::path &project_root,
    const porytiles2::TilesetArtifactKeyProvider *key_provider,
    const std::string &tileset)
{
    std::vector<std::filesystem::path> paths;

    // Get tileset-specific config paths using the key provider
    using enum porytiles2::TilesetArtifact::Type;
    const auto tileset_local_config_key = key_provider->key_for(tileset, porytiles2::TilesetArtifact{local_config});
    const auto tileset_config_key = key_provider->key_for(tileset, porytiles2::TilesetArtifact{config});

    // Priority order (highest to lowest):
    // 1. tileset_folder/config.local.yaml
    paths.push_back(std::filesystem::path{tileset_local_config_key.key()});

    // 2. tileset_folder/config.yaml
    paths.push_back(std::filesystem::path{tileset_config_key.key()});

    // 3. project_root/config.local.yaml
    paths.push_back(project_root / "porytiles.local.yaml");

    // 4. project_root/config.yaml
    paths.push_back(project_root / "porytiles.yaml");

    return paths;
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
 * @tparam ParseFunc Function type for parsing value (node, key, path -> LayerValue<T>)
 * @param paths Config file paths to search in priority order
 * @param load_func Function to load a YAML file
 * @param extract_node_func Function to extract the target node from the YAML doc
 * @param parse_func Function to parse the value from the node
 * @param key The configuration key name (for error messages)
 * @return The first valid LayerValue found, or not_provided if not found in any file
 */
template <typename T, typename LoadFunc, typename NodeExtractFunc, typename ParseFunc>
porytiles2::LayerValue<T> search_config_files(
    const std::vector<std::filesystem::path> &paths,
    LoadFunc load_func,
    NodeExtractFunc extract_node_func,
    ParseFunc parse_func,
    const std::string &key)
{
    using porytiles2::ValidationState;

    for (const auto &path : paths) {
        const auto yaml_doc = load_func(path);
        if (!yaml_doc.has_value()) {
            // File doesn't exist or couldn't be loaded, try next file
            continue;
        }

        try {
            const auto node = extract_node_func(yaml_doc.value());
            const auto result = parse_func(node, key, path.string());

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
    return porytiles2::LayerValue<T>::not_provided();
}

} // namespace

namespace porytiles2 {

YamlFileProvider::YamlFileProvider(
    const std::filesystem::path &project_root, const TilesetArtifactKeyProvider &tileset_key_provider)
    : project_root_{project_root}, key_provider_{&tileset_key_provider}
{
    // Config files are loaded lazily when first accessed via the anonymous namespace functions
}

std::string YamlFileProvider::name() const
{
    return "YamlFileProvider";
}

LayerValue<std::size_t> YamlFileProvider::num_tiles_primary(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["num_tiles_primary"]; },
        parse_size_t,
        "num_tiles_primary");
}

LayerValue<std::size_t> YamlFileProvider::num_tiles_total(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["num_tiles_total"]; },
        parse_size_t,
        "num_tiles_total");
}

LayerValue<std::size_t> YamlFileProvider::num_metatiles_primary(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["num_metatiles_primary"]; },
        parse_size_t,
        "num_metatiles_primary");
}

LayerValue<std::size_t> YamlFileProvider::num_metatiles_total(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["num_metatiles_total"]; },
        parse_size_t,
        "num_metatiles_total");
}

LayerValue<std::size_t> YamlFileProvider::num_pals_primary(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["num_pals_primary"]; },
        parse_size_t,
        "num_pals_primary");
}

LayerValue<std::size_t> YamlFileProvider::num_pals_total(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["num_pals_total"]; },
        parse_size_t,
        "num_pals_total");
}

LayerValue<std::size_t> YamlFileProvider::max_map_data_size(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["max_map_data_size"]; },
        parse_size_t,
        "max_map_data_size");
}

LayerValue<std::size_t> YamlFileProvider::num_tiles_per_metatile(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<std::size_t>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["fieldmap"]["num_tiles_per_metatile"]; },
        parse_size_t,
        "num_tiles_per_metatile");
}

LayerValue<Rgba32> YamlFileProvider::extrinsic_transparency(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<Rgba32>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["extrinsic_transparency"]; },
        parse_rgba32,
        "extrinsic_transparency");
}

LayerValue<bool> YamlFileProvider::patch_build_enabled(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<bool>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["patch"]["enabled"]; },
        parse_bool,
        "patch.enabled");
}

LayerValue<TilesPalMode> YamlFileProvider::tiles_pal_mode(const std::string &tileset) const
{
    const auto paths = get_config_paths(project_root_, key_provider_, tileset);
    return search_config_files<TilesPalMode>(
        paths,
        load_yaml_file,
        [](const YAML::Node &doc) { return doc["tiles_pal_mode"]; },
        parse_tiles_pal_mode,
        "tiles_pal_mode");
}

} // namespace porytiles2

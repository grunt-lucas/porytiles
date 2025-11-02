#include "porytiles2/infra/config/yaml_file_provider.hpp"

#include <cstdint>
#include <filesystem>
#include <sstream>

#include "fmt/format.h"

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
        const auto source = fmt::format("YAML file: {}", file_path);
        return porytiles2::LayerValue<std::size_t>::valid(value, source);
    }
    catch (const YAML::Exception &e) {
        const auto error = fmt::format("Failed to parse '{}' as std::size_t: {}", key, e.what());
        const auto source = fmt::format("YAML file: {}", file_path);
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
        const auto source = fmt::format("YAML file: {}", file_path);
        return porytiles2::LayerValue<bool>::valid(value, source);
    }
    catch (const YAML::Exception &e) {
        const auto error = fmt::format("Failed to parse '{}' as bool: {}", key, e.what());
        const auto source = fmt::format("YAML file: {}", file_path);
        return porytiles2::LayerValue<bool>::invalid(error, source);
    }
}

/**
 * @brief Attempts to parse an Rgba32 color from a YAML node.
 *
 * @details
 * Expects the YAML node to be a sequence with 3 or 4 elements: [r, g, b] or [r, g, b, a].
 * If alpha is not provided, it defaults to 255 (opaque).
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
        if (!node.IsSequence()) {
            const auto error = fmt::format("'{}' must be a sequence [r, g, b] or [r, g, b, a]", key);
            const auto source = fmt::format("YAML file: {}", file_path);
            return porytiles2::LayerValue<porytiles2::Rgba32>::invalid(error, source);
        }

        if (node.size() < 3 || node.size() > 4) {
            const auto error =
                fmt::format("'{}' must have 3 or 4 elements [r, g, b] or [r, g, b, a], got {}", key, node.size());
            const auto source = fmt::format("YAML file: {}", file_path);
            return porytiles2::LayerValue<porytiles2::Rgba32>::invalid(error, source);
        }

        const auto r = node[0].as<std::uint8_t>();
        const auto g = node[1].as<std::uint8_t>();
        const auto b = node[2].as<std::uint8_t>();
        const auto a = (node.size() == 4) ? node[3].as<std::uint8_t>() : porytiles2::Rgba32::alpha_opaque;

        const porytiles2::Rgba32 color{r, g, b, a};
        const auto source = fmt::format("YAML file: {}", file_path);
        return porytiles2::LayerValue<porytiles2::Rgba32>::valid(color, source);
    }
    catch (const YAML::Exception &e) {
        const auto error = fmt::format("Failed to parse '{}' as Rgba32: {}", key, e.what());
        const auto source = fmt::format("YAML file: {}", file_path);
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
        const auto str = node.as<std::string>();
        const auto mode_opt = porytiles2::tiles_pal_mode_from_str(str);

        if (!mode_opt.has_value()) {
            const auto error =
                fmt::format("'{}' has invalid value '{}', expected 'true-color' or 'greyscale'", key, str);
            const auto source = fmt::format("YAML file: {}", file_path);
            return porytiles2::LayerValue<porytiles2::TilesPalMode>::invalid(error, source);
        }

        const auto source = fmt::format("YAML file: {}", file_path);
        return porytiles2::LayerValue<porytiles2::TilesPalMode>::valid(mode_opt.value(), source);
    }
    catch (const YAML::Exception &e) {
        const auto error = fmt::format("Failed to parse '{}' as TilesPalMode: {}", key, e.what());
        const auto source = fmt::format("YAML file: {}", file_path);
        return porytiles2::LayerValue<porytiles2::TilesPalMode>::invalid(error, source);
    }
}

} // namespace

namespace porytiles2 {

/*
 * TODO: Refactor Idea
 * Instead of having one YamlFileProvider for each YAML file path we want to load, perhaps we should instead have one
 * YamlFileProvider instance that takes some set of path providers (think ProjectTilesetKeyProvider) which allow it to
 * find the config path for a given tileset, and optionally load that file if present. It can keep a cache of loaded
 * files for later lookup. If the tileset or layout specific config file doesn't exist, it checks the standard global
 * config location and sees if that has the setting. If that doesn't exist, it returns the not_provided value.
 */

YamlFileProvider::YamlFileProvider(const std::string &yaml_file_path) : file_path_{yaml_file_path}
{
    if (!std::filesystem::exists(yaml_file_path)) {
        // File doesn't exist, yaml_doc_ remains empty (nullopt)
        return;
    }

    try {
        yaml_doc_ = YAML::LoadFile(yaml_file_path);
    }
    catch (const YAML::Exception &) {
        // Failed to parse YAML, yaml_doc_ remains empty (nullopt)
        // All methods will return not_provided
    }
}

std::string YamlFileProvider::name() const
{
    return "YamlFileProvider";
}

LayerValue<std::size_t> YamlFileProvider::num_tiles_primary(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["num_tiles_primary"];
    return parse_size_t(node, "num_tiles_primary", file_path_);
}

LayerValue<std::size_t> YamlFileProvider::num_tiles_total(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["num_tiles_total"];
    return parse_size_t(node, "num_tiles_total", file_path_);
}

LayerValue<std::size_t> YamlFileProvider::num_metatiles_primary(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["num_metatiles_primary"];
    return parse_size_t(node, "num_metatiles_primary", file_path_);
}

LayerValue<std::size_t> YamlFileProvider::num_metatiles_total(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["num_metatiles_total"];
    return parse_size_t(node, "num_metatiles_total", file_path_);
}

LayerValue<std::size_t> YamlFileProvider::num_pals_primary(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["num_pals_primary"];
    return parse_size_t(node, "num_pals_primary", file_path_);
}

LayerValue<std::size_t> YamlFileProvider::num_pals_total(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["num_pals_total"];
    return parse_size_t(node, "num_pals_total", file_path_);
}

LayerValue<std::size_t> YamlFileProvider::max_map_data_size(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["max_map_data_size"];
    return parse_size_t(node, "max_map_data_size", file_path_);
}

LayerValue<std::size_t> YamlFileProvider::num_tiles_per_metatile(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<std::size_t>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["num_tiles_per_metatile"];
    return parse_size_t(node, "num_tiles_per_metatile", file_path_);
}

LayerValue<Rgba32> YamlFileProvider::extrinsic_transparency(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<Rgba32>::not_provided();
    }

    const auto node = yaml_doc_.value()["fieldmap"]["extrinsic_transparency"];
    return parse_rgba32(node, "extrinsic_transparency", file_path_);
}

LayerValue<bool> YamlFileProvider::patch_build_enabled(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<bool>::not_provided();
    }

    const auto node = yaml_doc_.value()["patch"]["enabled"];
    return parse_bool(node, "patch.enabled", file_path_);
}

LayerValue<TilesPalMode> YamlFileProvider::tiles_pal_mode(const std::string &tileset) const
{
    if (!yaml_doc_.has_value()) {
        return LayerValue<TilesPalMode>::not_provided();
    }

    const auto node = yaml_doc_.value()["tiles_pal_mode"];
    return parse_tiles_pal_mode(node, "tiles_pal_mode", file_path_);
}

} // namespace porytiles2

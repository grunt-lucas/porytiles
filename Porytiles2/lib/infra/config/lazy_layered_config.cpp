#include "porytiles2/infra/config/lazy_layered_config.hpp"

#include <any>
#include <functional>
#include <ranges>
#include <string>

#include "fmt/format.h"

#include "porytiles2/app/config/incremental_build_mode.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"
#include "porytiles2/utilities/source_locations.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

template <typename T>
ConfigValue<T> LazyLayeredConfig::resolve_config_value(
    const std::string &cache_key, std::function<LayerValue<T>(const ConfigProvider &)> provider_call) const
{
    /*
     * WARNING! HACK ALERT! WARNING!
     * DO NOT DELETE THIS USING STATEMENT.
     *
     * We need to declare this here so that the to_string call below can resolve to either the std:: version or one of
     * our custom versions automagically.
     */
    using std::to_string;

    // Check if already cached
    if (cache_.contains(cache_key)) {
        // TODO: catch bad_any_cast here and panic
        T cached_value = std::any_cast<T>(cache_.at(cache_key));
        std::string source = provenance_.at(cache_key);
        return ConfigValue<T>{cached_value, cache_key, source};
    }

    // Search through providers in priority order
    for (const auto &provider : providers_) {
        if (LayerValue<T> layer_value = provider_call(*provider); layer_value.value.has_value()) {
            T resolved_value = layer_value.value.value();
            cache_[cache_key] = resolved_value;
            cache_value_strings_[cache_key] = to_string(resolved_value);
            std::string source = fmt::format("{}: {}", provider->name(), layer_value.metadata);
            provenance_[cache_key] = source;
            return ConfigValue<T>{resolved_value, cache_key, source};
        }
    }

    // No value found in any layer - this is a programmer error
    panic(fmt::format("no value found for {} in any config layer", cache_key));
}

ConfigValue<std::size_t> LazyLayeredConfig::num_tiles_primary(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_tiles_primary(tileset); });
}

ConfigValue<std::size_t> LazyLayeredConfig::num_tiles_total(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_tiles_total(tileset); });
}

ConfigValue<std::size_t> LazyLayeredConfig::num_metatiles_primary(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_metatiles_primary(tileset); });
}

ConfigValue<std::size_t> LazyLayeredConfig::num_metatiles_total(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_metatiles_total(tileset); });
}

ConfigValue<std::size_t> LazyLayeredConfig::num_pals_primary(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_pals_primary(tileset); });
}

ConfigValue<std::size_t> LazyLayeredConfig::num_pals_total(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_pals_total(tileset); });
}

ConfigValue<std::size_t> LazyLayeredConfig::max_map_data_size(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.max_map_data_size(tileset); });
}

ConfigValue<std::size_t> LazyLayeredConfig::num_tiles_per_metatile(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_tiles_per_metatile(tileset); });
}

ConfigValue<Rgba32> LazyLayeredConfig::extrinsic_transparency(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<Rgba32>(
        key, [&tileset](const ConfigProvider &provider) { return provider.extrinsic_transparency(tileset); });
}

ConfigValue<IncrementalBuildMode> LazyLayeredConfig::incremental_build_mode(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<IncrementalBuildMode>(
        key, [&tileset](const ConfigProvider &provider) { return provider.incremental_build_mode(tileset); });
}

ConfigValue<TilesPalMode> LazyLayeredConfig::tiles_pal_mode(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<TilesPalMode>(
        key, [&tileset](const ConfigProvider &provider) { return provider.tiles_pal_mode(tileset); });
}

std::string LazyLayeredConfig::dump() const
{
    if (cache_.empty()) {
        return "LazyLayeredConfig {}";
    }

    std::string result = "LazyLayeredConfig {\n";
    for (const auto &key : cache_ | std::views::keys) {
        const auto provenance_it = provenance_.find(key);
        const std::string provenance_info =
            (provenance_it != provenance_.end()) ? provenance_it->second : "<unknown source>";

        const auto value_it = cache_value_strings_.find(key);
        const std::string value_str = (value_it != cache_value_strings_.end()) ? value_it->second : "<unknown value>";

        result += fmt::format("  {} = {} [{}]\n", key, value_str, provenance_info);
    }
    result += "}\n";

    return result;
}

void LazyLayeredConfig::warmup_cache(const std::vector<std::string> &tileset_names) const
{
    for (const auto &tileset_name : tileset_names) {
        std::ignore = incremental_build_mode(tileset_name);
        std::ignore = tiles_pal_mode(tileset_name);
        std::ignore = num_tiles_primary(tileset_name);
        std::ignore = num_tiles_total(tileset_name);
        std::ignore = num_metatiles_primary(tileset_name);
        std::ignore = num_metatiles_total(tileset_name);
        std::ignore = num_pals_primary(tileset_name);
        std::ignore = num_pals_total(tileset_name);
        std::ignore = max_map_data_size(tileset_name);
        std::ignore = num_tiles_per_metatile(tileset_name);
    }
}

} // namespace porytiles2

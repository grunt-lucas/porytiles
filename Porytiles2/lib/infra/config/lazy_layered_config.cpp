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
ChainableResult<ConfigValue<T>> LazyLayeredConfig::resolve_config_value(
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
        LayerValue<T> layer_value = provider_call(*provider);

        // Check if provider found an invalid value - stop immediately and return error
        if (layer_value.state == ValidationState::invalid) {
            const auto source_info = format_->format("{}", layer_value.source_info);
            return FormattableError{format_->format(
                "{} <- {}: invalid value: {}",
                FormatParam{cache_key, Style::bold},
                FormatParam{source_info, Style::bold},
                layer_value.error_message)};
        }

        // Check if provider has a valid value
        if (layer_value.state == ValidationState::valid) {
            T resolved_value = layer_value.value.value();
            cache_[cache_key] = resolved_value;
            cache_value_strings_[cache_key] = to_string(resolved_value);
            std::string source = fmt::format("{}", layer_value.source_info);
            provenance_[cache_key] = source;
            return ConfigValue<T>{resolved_value, cache_key, source};
        }

        // Otherwise, state is not_provided - continue to next provider
    }

    // No value found in any layer - this is a programmer error
    panic(fmt::format("no value found for '{}' in any config layer", cache_key));
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::num_tiles_primary(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_tiles_primary(tileset); });
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::num_tiles_total(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_tiles_total(tileset); });
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::num_metatiles_primary(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_metatiles_primary(tileset); });
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::num_metatiles_total(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_metatiles_total(tileset); });
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::num_pals_primary(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_pals_primary(tileset); });
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::num_pals_total(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_pals_total(tileset); });
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::max_map_data_size(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.max_map_data_size(tileset); });
}

ChainableResult<ConfigValue<std::size_t>> LazyLayeredConfig::num_tiles_per_metatile(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<std::size_t>(
        key, [&tileset](const ConfigProvider &provider) { return provider.num_tiles_per_metatile(tileset); });
}

ChainableResult<ConfigValue<Rgba32>> LazyLayeredConfig::extrinsic_transparency(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<Rgba32>(
        key, [&tileset](const ConfigProvider &provider) { return provider.extrinsic_transparency(tileset); });
}

ChainableResult<ConfigValue<bool>> LazyLayeredConfig::patch_build_enabled(const std::string &tileset) const
{
    const auto name = extract_function_name();
    const auto key = tileset + ":" + name;
    return resolve_config_value<bool>(
        key, [&tileset](const ConfigProvider &provider) { return provider.patch_build_enabled(tileset); });
}

ChainableResult<ConfigValue<TilesPalMode>> LazyLayeredConfig::tiles_pal_mode(const std::string &tileset) const
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
        // Note: These calls now return ChainableResult, but we ignore both success and error cases
        // This is intentional for warmup - we just want to populate the cache
        // If you want to validate config at startup, call these and check has_value()

        // Domain config
        std::ignore = num_tiles_primary(tileset_name);
        std::ignore = num_tiles_total(tileset_name);
        std::ignore = num_metatiles_primary(tileset_name);
        std::ignore = num_metatiles_total(tileset_name);
        std::ignore = num_pals_primary(tileset_name);
        std::ignore = num_pals_total(tileset_name);
        std::ignore = max_map_data_size(tileset_name);
        std::ignore = num_tiles_per_metatile(tileset_name);
        std::ignore = extrinsic_transparency(tileset_name);
        std::ignore = patch_build_enabled(tileset_name);

        // App config
        // TODO: stuff

        // Infra config
        std::ignore = tiles_pal_mode(tileset_name);
    }
}

} // namespace porytiles2

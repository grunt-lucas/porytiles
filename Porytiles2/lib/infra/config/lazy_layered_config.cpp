#include "porytiles2/infra/config/lazy_layered_config.hpp"

#include <any>
#include <functional>
#include <ranges>
#include <string>

#include "fmt/format.h"

#include "porytiles2/domain/config/incremental_build_mode.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

template <typename T>
T LazyLayeredConfig::resolve_config_value(
    const std::string &cache_key, std::function<LayerValue<T>(const ConfigLayerProvider &)> provider_call) const {
    /*
     * We need to declare this here to the to_string call below can resolve to either the std:: version or one of our
     * custom versions.
     */
    using std::to_string;

    // Check if already cached
    if (cache_.contains(cache_key)) {
        // TODO: catch bad_any_cast here and panic
        return std::any_cast<T>(cache_.at(cache_key));
    }

    // Search through providers in priority order
    for (const auto &provider : providers_) {
        if (LayerValue<T> layer_value = provider_call(*provider); layer_value.value.has_value()) {
            T resolved_value = layer_value.value.value();
            cache_[cache_key] = resolved_value;
            cache_values_[cache_key] = to_string(resolved_value);
            provenance_[cache_key] = fmt::format("{}: {}", provider->name(), layer_value.metadata);
            return resolved_value;
        }
    }

    // No value found in any layer - this is a programmer error
    panic(fmt::format("no value found for {} in any config layer", cache_key));
}

std::size_t LazyLayeredConfig::num_tiles_primary() const {
    return resolve_config_value<std::size_t>(fmt::format("num_tiles_primary"), [](const ConfigLayerProvider &provider) {
        return provider.num_tiles_primary();
    });
}

std::size_t LazyLayeredConfig::num_tiles_total() const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_tiles_total"), [](const ConfigLayerProvider &provider) { return provider.num_tiles_total(); });
}

std::size_t LazyLayeredConfig::num_metatiles_primary() const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_metatiles_primary"),
        [](const ConfigLayerProvider &provider) { return provider.num_metatiles_primary(); });
}

std::size_t LazyLayeredConfig::num_metatiles_total() const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_metatiles_total"),
        [](const ConfigLayerProvider &provider) { return provider.num_metatiles_total(); });
}

std::size_t LazyLayeredConfig::num_pals_primary() const {
    return resolve_config_value<std::size_t>(fmt::format("num_pals_primary"), [](const ConfigLayerProvider &provider) {
        return provider.num_pals_primary();
    });
}

std::size_t LazyLayeredConfig::num_pals_total() const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_pals_total"), [](const ConfigLayerProvider &provider) { return provider.num_pals_total(); });
}

std::size_t LazyLayeredConfig::max_map_data_size() const {
    return resolve_config_value<std::size_t>(fmt::format("max_map_data_size"), [](const ConfigLayerProvider &provider) {
        return provider.max_map_data_size();
    });
}

std::size_t LazyLayeredConfig::num_tiles_per_metatile() const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_tiles_per_metatile"),
        [](const ConfigLayerProvider &provider) { return provider.num_tiles_per_metatile(); });
}

IncrementalBuildMode LazyLayeredConfig::incremental_build_mode(const std::string &tileset_name) const {
    return resolve_config_value<IncrementalBuildMode>(
        fmt::format("incremental_build_mode:{}", tileset_name),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.incremental_build_mode(tileset_name); });
}

std::string LazyLayeredConfig::dump() const {
    if (cache_.empty()) {
        return "LazyLayeredConfig {}";
    }

    std::string result = "LazyLayeredConfig {\n";
    for (const auto &key : cache_ | std::views::keys) {
        const auto provenance_it = provenance_.find(key);
        const std::string provenance_info =
            (provenance_it != provenance_.end()) ? provenance_it->second : "<unknown source>";

        const auto value_it = cache_values_.find(key);
        const std::string value_str = (value_it != cache_values_.end()) ? value_it->second : "<unknown value>";

        result += fmt::format("  {} = {} [{}]\n", key, value_str, provenance_info);
    }
    result += "}\n";

    return result;
}

void LazyLayeredConfig::warmup_cache(const std::vector<std::string> &tileset_names) const {
    // Cache tileset-specific values for each provided tileset
    for (const auto &tileset_name : tileset_names) {
        std::ignore = incremental_build_mode(tileset_name);
    }

    // Cache global (non-tileset-specific) values
    std::ignore = num_tiles_primary();
    std::ignore = num_tiles_total();
    std::ignore = num_metatiles_primary();
    std::ignore = num_metatiles_total();
    std::ignore = num_pals_primary();
    std::ignore = num_pals_total();
    std::ignore = max_map_data_size();
    std::ignore = num_tiles_per_metatile();
}

} // namespace porytiles2

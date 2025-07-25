#include "porytiles2/infra/config/lazy_layered_config.hpp"

#include <any>
#include <functional>

#include "fmt/format.h"

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

template <typename T>
T LazyLayeredConfig::resolve_config_value(
    const std::string &cache_key, std::function<LayerValue<T>(const ConfigLayerProvider &)> provider_call) const {
    // Check if already cached
    if (cache_.contains(cache_key)) {
        // TODO: catch bad_any_cast here and panic
        return std::any_cast<T>(cache_.at(cache_key));
    }

    // Search through providers in priority order
    for (const auto &provider : providers_) {
        if (LayerValue<T> layer_value = provider_call(*provider); layer_value.value.has_value()) {
            cache_[cache_key] = layer_value.value.value();
            provenance_[cache_key] = fmt::format("{}: {}", provider->name(), layer_value.metadata);
            return layer_value.value.value();
        }
    }

    // No value found in any layer - this is a programmer error
    panic(fmt::format("no value found for {} in any config layer.", cache_key));
}

std::size_t LazyLayeredConfig::num_tiles_primary(const std::string &tileset_name) const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_tiles_primary:{}", tileset_name),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.num_tiles_primary(tileset_name); });
}

std::size_t LazyLayeredConfig::num_tiles_total(const std::string &tileset_name) const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_tiles_total:{}", tileset_name),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.num_tiles_total(tileset_name); });
}

std::size_t LazyLayeredConfig::num_metatiles_primary(const std::string &tileset_name) const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_metatiles_primary:{}", tileset_name),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.num_metatiles_primary(tileset_name); });
}

std::size_t LazyLayeredConfig::num_metatiles_total(const std::string &tileset_name) const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_metatiles_total:{}", tileset_name),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.num_metatiles_total(tileset_name); });
}

std::size_t LazyLayeredConfig::num_pals_primary(const std::string &tileset_name) const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_pals_primary:{}", tileset_name),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.num_pals_primary(tileset_name); });
}

std::size_t LazyLayeredConfig::num_pals_total(const std::string &tileset_name) const {
    return resolve_config_value<std::size_t>(
        fmt::format("num_pals_total:{}", tileset_name),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.num_pals_total(tileset_name); });
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
        fmt::format("incremental_build_mode"),
        [&tileset_name](const ConfigLayerProvider &provider) { return provider.incremental_build_mode(tileset_name); });
}

} // namespace porytiles2

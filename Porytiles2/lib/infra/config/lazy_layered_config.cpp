#include "porytiles2/infra/config/lazy_layered_config.hpp"

#include <any>

namespace porytiles2 {

std::size_t LazyLayeredConfig::num_tiles_primary(const std::string &tileset_name) const {
    // Create a unique cache key for this value
    std::string cache_key = fmt::format("num_tiles_primary:{}", tileset_name);

    // Check if already cached
    if (cache_.contains(cache_key)) {
        // TODO: catch bad_any_cast here and panic
        return std::any_cast<std::size_t>(cache_.at(cache_key));
    }

    // Search through providers in priority order
    for (const auto &provider : providers_) {
        LayerValue<std::size_t> layer_value = provider->num_tiles_primary(tileset_name);

        if (layer_value.value.has_value()) {
            cache_[cache_key] = layer_value.value.value();
            provenance_[cache_key] = fmt::format("{}: {}", provider->name(), layer_value.metadata);
            return layer_value.value.value();
        }
    }

    // No value found in any layer - this is a programmer error
    panic(fmt::format("No value found for num_tiles_primary({}) in any config layer.", tileset_name));
}

std::size_t LazyLayeredConfig::num_tiles_total(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

std::size_t LazyLayeredConfig::num_metatiles_primary(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

std::size_t LazyLayeredConfig::num_metatiles_total(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

std::size_t LazyLayeredConfig::num_pals_primary(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

std::size_t LazyLayeredConfig::num_pals_total(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

std::size_t LazyLayeredConfig::max_map_data_size() const {
    panic("TODO: unimplemented");
}

std::size_t LazyLayeredConfig::num_tiles_per_metatile() const {
    panic("TODO: unimplemented");
}

IncrementalBuildMode LazyLayeredConfig::incremental_build_mode(const std::string &tileset_name) const {
    panic("TODO: unimplemented");
}

} // namespace porytiles2

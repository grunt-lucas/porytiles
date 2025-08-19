#include "porytiles2/infra/config/config_provider.hpp"

namespace porytiles2 {

LayerValue<std::size_t> ConfigProvider::num_tiles_primary() const {
    return LayerValue<std::size_t>{};
}

LayerValue<std::size_t> ConfigProvider::num_tiles_total() const {
    return LayerValue<std::size_t>{};
}

LayerValue<std::size_t> ConfigProvider::num_metatiles_primary() const {
    return LayerValue<std::size_t>{};
}

LayerValue<std::size_t> ConfigProvider::num_metatiles_total() const {
    return LayerValue<std::size_t>{};
}

LayerValue<std::size_t> ConfigProvider::num_pals_primary() const {
    return LayerValue<std::size_t>{};
}

LayerValue<std::size_t> ConfigProvider::num_pals_total() const {
    return LayerValue<std::size_t>{};
}

LayerValue<std::size_t> ConfigProvider::max_map_data_size() const {
    return LayerValue<std::size_t>{};
}

LayerValue<std::size_t> ConfigProvider::num_tiles_per_metatile() const {
    return LayerValue<std::size_t>{};
}

LayerValue<IncrementalBuildMode> ConfigProvider::incremental_build_mode(const std::string &tileset_name) const {
    return LayerValue<IncrementalBuildMode>{};
}

LayerValue<TilesPalMode> ConfigProvider::tiles_pal_mode(const std::string &tileset_name) const {
    return LayerValue<TilesPalMode>{};
}

} // namespace porytiles2

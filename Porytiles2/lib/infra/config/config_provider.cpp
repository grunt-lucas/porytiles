#include "porytiles2/infra/config/config_provider.hpp"

namespace porytiles2 {

LayerValue<std::size_t> ConfigProvider::num_tiles_primary(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<std::size_t> ConfigProvider::num_tiles_total(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<std::size_t> ConfigProvider::num_metatiles_primary(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<std::size_t> ConfigProvider::num_metatiles_total(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<std::size_t> ConfigProvider::num_pals_primary(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<std::size_t> ConfigProvider::num_pals_total(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<std::size_t> ConfigProvider::max_map_data_size(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<std::size_t> ConfigProvider::num_tiles_per_metatile(const std::string &tileset) const
{
    return LayerValue<std::size_t>::not_provided();
}

LayerValue<Rgba32> ConfigProvider::extrinsic_transparency(const std::string &tileset) const
{
    return LayerValue<Rgba32>::not_provided();
}

LayerValue<IncrementalBuildMode> ConfigProvider::incremental_build_mode(const std::string &tileset) const
{
    return LayerValue<IncrementalBuildMode>::not_provided();
}

LayerValue<TilesPalMode> ConfigProvider::tiles_pal_mode(const std::string &tileset) const
{
    return LayerValue<TilesPalMode>::not_provided();
}

} // namespace porytiles2

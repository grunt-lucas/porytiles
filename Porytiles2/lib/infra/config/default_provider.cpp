#include "porytiles2/infra/config/default_provider.hpp"

namespace porytiles2 {

std::string DefaultProvider::name() const
{
    return "Default Provider";
}

LayerValue<std::size_t> DefaultProvider::num_tiles_primary(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(512, "default value");
}

LayerValue<std::size_t> DefaultProvider::num_tiles_total(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(1024, "default value");
}

LayerValue<std::size_t> DefaultProvider::num_metatiles_primary(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(512, "default value");
}

LayerValue<std::size_t> DefaultProvider::num_metatiles_total(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(1024, "default value");
}

LayerValue<std::size_t> DefaultProvider::num_pals_primary(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(6, "default value");
}

LayerValue<std::size_t> DefaultProvider::num_pals_total(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(13, "default value");
}

LayerValue<std::size_t> DefaultProvider::max_map_data_size(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(10240, "default value");
}

LayerValue<std::size_t> DefaultProvider::num_tiles_per_metatile(const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(8, "default value");
}

LayerValue<Rgba32> DefaultProvider::extrinsic_transparency(const std::string &tileset) const
{
    return LayerValue<Rgba32>::valid(rgba_magenta, "default value");
}

LayerValue<IncrementalBuildMode> DefaultProvider::incremental_build_mode(const std::string &tileset) const
{
    return LayerValue<IncrementalBuildMode>::valid(IncrementalBuildMode::off, "default value");
}

LayerValue<TilesPalMode> DefaultProvider::tiles_pal_mode(const std::string &tileset) const
{
    return LayerValue<TilesPalMode>::valid(TilesPalMode::true_color, "default value");
}

} // namespace porytiles2
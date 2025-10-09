#include "porytiles2/infra/config/default_provider.hpp"

namespace porytiles2 {

std::string DefaultProvider::name() const
{
    return "DefaultProvider";
}

LayerValue<std::size_t> DefaultProvider::num_tiles_primary() const
{
    return {512, "default value"};
}

LayerValue<std::size_t> DefaultProvider::num_tiles_total() const
{
    return {1024, "default value"};
}

LayerValue<std::size_t> DefaultProvider::num_metatiles_primary() const
{
    return {512, "default value"};
}

LayerValue<std::size_t> DefaultProvider::num_metatiles_total() const
{
    return {1024, "default value"};
}

LayerValue<std::size_t> DefaultProvider::num_pals_primary() const
{
    return {6, "default value"};
}

LayerValue<std::size_t> DefaultProvider::num_pals_total() const
{
    return {13, "default value"};
}

LayerValue<std::size_t> DefaultProvider::max_map_data_size() const
{
    return {10240, "default value"};
}

LayerValue<std::size_t> DefaultProvider::num_tiles_per_metatile() const
{
    return {8, "default value"};
}

LayerValue<Rgba32> DefaultProvider::extrinsic_transparency() const
{
    return {rgba_magenta, "default value"};
}

LayerValue<IncrementalBuildMode> DefaultProvider::incremental_build_mode(const std::string &tileset_name) const
{
    return {IncrementalBuildMode::off, "default value"};
}

LayerValue<TilesPalMode> DefaultProvider::tiles_pal_mode(const std::string &tileset_name) const
{
    return {TilesPalMode::true_color, "default value"};
}

} // namespace porytiles2
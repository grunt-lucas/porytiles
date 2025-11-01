#include "porytiles2/infra/config/default_provider.hpp"

namespace {

const std::string source_info = "default value";

}

namespace porytiles2 {

std::string DefaultProvider::name() const
{
    return "DefaultProvider";
}

LayerValue<std::size_t> DefaultProvider::num_tiles_primary([[maybe_unused]] const std::string &tileset) const
{
    // return LayerValue<std::size_t>::invalid("value must be <= 512", "default");
    return LayerValue<std::size_t>::valid(512, source_info);
}

LayerValue<std::size_t> DefaultProvider::num_tiles_total([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(1024, source_info);
}

LayerValue<std::size_t> DefaultProvider::num_metatiles_primary([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(512, source_info);
}

LayerValue<std::size_t> DefaultProvider::num_metatiles_total([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(1024, source_info);
}

LayerValue<std::size_t> DefaultProvider::num_pals_primary([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(6, source_info);
}

LayerValue<std::size_t> DefaultProvider::num_pals_total([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(13, source_info);
}

LayerValue<std::size_t> DefaultProvider::max_map_data_size([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(10240, source_info);
}

LayerValue<std::size_t> DefaultProvider::num_tiles_per_metatile([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<std::size_t>::valid(8, source_info);
}

LayerValue<Rgba32> DefaultProvider::extrinsic_transparency([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<Rgba32>::valid(rgba_magenta, source_info);
}

LayerValue<bool> DefaultProvider::patch_build_enabled([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<bool>::valid(false, source_info);
}

LayerValue<TilesPalMode> DefaultProvider::tiles_pal_mode([[maybe_unused]] const std::string &tileset) const
{
    return LayerValue<TilesPalMode>::valid(TilesPalMode::true_color, source_info);
}

} // namespace porytiles2
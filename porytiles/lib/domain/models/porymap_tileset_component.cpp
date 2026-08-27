#include "porytiles/domain/models/porymap_tileset_component.hpp"

#include <format>
#include <utility>

#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/domain/services/tile_printer.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

PorymapTilesetComponent::PorymapTilesetComponent()
{
    palettes_.fill(Palette<Rgba32, palette::max_size>{Rgba32{}});
}

void PorymapTilesetComponent::push_back_tilemap_entry(TilemapEntry entry)
{
    // std::move here even though TilemapEntry is trivially-copyable, in case it changes later
    metatiles_bin_.push_back(std::move(entry));
}

void PorymapTilesetComponent::push_back_attribute(MetatileAttribute attribute)
{
    metatile_attributes_.push_back(std::move(attribute));
}

void PorymapTilesetComponent::set_palette(std::size_t palette_index, Palette<Rgba32, palette::max_size> palette)
{
    if (palette_index >= palette::num_palettes) {
        panic(std::format("invalid palette index {}: out of range", palette_index));
    }
    palettes_.at(palette_index) = std::move(palette);
}

const Palette<Rgba32, palette::max_size> &PorymapTilesetComponent::palette_at(std::size_t palette_index) const
{
    if (palette_index >= palette::num_palettes) {
        panic(std::format("invalid palette index {}: out of range", palette_index));
    }
    return palettes_.at(palette_index);
}

bool PorymapTilesetComponent::is_empty() const
{
    return metatiles_bin_.empty();
}

ChainableResult<LayerMode> PorymapTilesetComponent::detect_layer_mode() const
{
    const auto &entries = metatiles_bin();
    const auto &attributes = metatile_attributes_bin();
    const std::size_t metatile_count = attributes.size();

    if (entries.size() % metatile::entries_per_metatile_triple == 0 &&
        entries.size() / metatile::entries_per_metatile_triple == metatile_count) {
        return LayerMode::triple;
    }
    if (entries.size() % metatile::entries_per_metatile_dual == 0 &&
        entries.size() / metatile::entries_per_metatile_dual == metatile_count) {
        return LayerMode::dual;
    }

    return FormattableError{
        std::vector<std::string>{
            "unexpected tilemap entry count in metatiles.bin",
            "found {} tilemap entries for {} metatile attributes",
            "for dual layer metatiles, expected {} entries ({} per metatile)",
            "for triple layer metatiles, expected {} entries ({} per metatile)"},
        std::vector<std::vector<FormatParam>>{
            {},
            {FormatParam{entries.size(), Style::bold}, FormatParam{metatile_count, Style::bold}},
            {FormatParam{metatile_count * metatile::entries_per_metatile_dual, Style::bold},
             metatile::entries_per_metatile_dual},
            {FormatParam{metatile_count * metatile::entries_per_metatile_triple, Style::bold},
             metatile::entries_per_metatile_triple}}};
}

void PorymapTilesetComponent::add_anim(Animation<IndexPixel> anim)
{
    const std::string &name = anim.name();
    if (anims_.contains(name)) {
        panic("animation '" + name + "' already exists in PorymapTilesetComponent");
    }
    anims_.insert({name, std::move(anim)});
}

} // namespace porytiles

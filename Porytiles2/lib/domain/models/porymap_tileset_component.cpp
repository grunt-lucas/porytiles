#include "porytiles2/domain/models/porymap_tileset_component.hpp"

#include <utility>

#include "fmt/format.h"

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

void PorymapTilesetComponent::push_back_tilemap_entry(TilemapEntry entry)
{
    // std::move here even though TilemapEntry is trivially-copyable, in case it changes later
    metatiles_bin_.push_back(std::move(entry));
}

void PorymapTilesetComponent::push_back_attribute(MetatileAttribute attribute)
{
    metatile_attributes_.push_back(std::move(attribute));
}

void PorymapTilesetComponent::set_pal(Palette<Rgba32> pal, unsigned int pal_index)
{
    if (pal_index >= pal::num_pals) {
        panic(fmt::format("invalid pal index {}: out of range", pal_index));
    }
    pals_[pal_index] = std::move(pal);
}

const Palette<Rgba32> &PorymapTilesetComponent::pal_at(unsigned int pal_index) const
{
    if (pal_index >= pal::num_pals) {
        panic(fmt::format("invalid pal index {}: out of range", pal_index));
    }
    return pals_[pal_index];
}

bool PorymapTilesetComponent::is_empty() const
{
    return metatiles_bin_.empty();
}

ChainableResult<tileset::LayerMode> PorymapTilesetComponent::detect_layer_mode() const
{
    const auto &entries = metatiles_bin();
    const auto &attributes = metatile_attributes_bin();
    const std::size_t metatile_count = attributes.size();

    if (entries.size() % metatile::entries_per_metatile_triple == 0 &&
        entries.size() / metatile::entries_per_metatile_triple == metatile_count) {
        return tileset::LayerMode::triple;
    }
    if (entries.size() % metatile::entries_per_metatile_dual == 0 &&
        entries.size() / metatile::entries_per_metatile_dual == metatile_count) {
        return tileset::LayerMode::dual;
    }

    // TODO: only expectation message associated with configured layer type?
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

} // namespace porytiles2

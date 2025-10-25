#include "porytiles2/domain/models/porymap_tileset_component.hpp"

#include <utility>

#include "fmt/format.h"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/rgba_pal.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
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

void PorymapTilesetComponent::set_pal(RgbaPal pal, unsigned int pal_index)
{
    if (pal_index >= pal::num_pals) {
        panic(fmt::format("invalid pal index {}: out of range", pal_index));
    }
    pals_[pal_index] = std::move(pal);
}

const RgbaPal &PorymapTilesetComponent::pal_at(unsigned int pal_index) const
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

    // TODO: don't hardcode 12 and 8 here
    if (entries.size() % 12 == 0 && entries.size() / 12 == metatile_count) {
        return tileset::LayerMode::triple;
    }
    if (entries.size() % 8 == 0 && entries.size() / 8 == metatile_count) {
        return tileset::LayerMode::dual;
    }

    return FormattableError{
        std::vector<std::string>{
            "metatiles.bin size did not correspond to metatile_attributes.bin size",
            "found {} tilemap entries and {} metatile attributes",
            "for dual layer metatiles, expected {} entries (8 per metatile)",
            "for triple layer metatiles, expected {} entries (12 per metatile)"},
        std::vector<std::vector<FormatParam>>{
            {},
            {FormatParam{entries.size(), Style::bold}, FormatParam{metatile_count, Style::bold}},
            {FormatParam{metatile_count * 8, Style::bold}},
            {FormatParam{metatile_count * 12, Style::bold}}}};
}

} // namespace porytiles2

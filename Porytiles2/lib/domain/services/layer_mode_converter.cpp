#include "porytiles2/domain/services/layer_mode_converter.hpp"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/panic/panic.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<metatile::LayerMode> LayerModeConverter::detect_layer_mode(
    const std::vector<TilemapEntry> &entries, const std::vector<MetatileAttribute> &attributes)
{
    const std::size_t metatile_count = attributes.size();

    if (entries.size() % 12 == 0 && entries.size() / 12 == metatile_count) {
        return metatile::LayerMode::triple;
    }
    if (entries.size() % 8 == 0 && entries.size() / 8 == metatile_count) {
        return metatile::LayerMode::dual;
    }

    diag_->err("failed to detect layer mode from metatiles.bin and metatile_attributes.bin");
    diag_->note({
        format_->format("found {} tilemap entries and {} metatile attributes", entries.size(), metatile_count),
        format_->format("for dual layer mode, expected {} entries (8 per metatile)", metatile_count * 8),
        format_->format("for triple layer mode, expected {} entries (12 per metatile)", metatile_count * 12),
    });
    return FormattableError{"layer mode detection failed"};
}

ChainableResult<std::vector<TilemapEntry>> LayerModeConverter::triple_layerize(
    const std::vector<TilemapEntry> &entries, const std::vector<MetatileAttribute> &attributes)
{
    panic("TODO: implement");
}

ChainableResult<std::vector<TilemapEntry>> LayerModeConverter::dual_layerize(
    const std::vector<TilemapEntry> &entries, const std::vector<MetatileAttribute> &attributes)
{
    panic("TODO: implement");
}

} // namespace porytiles2

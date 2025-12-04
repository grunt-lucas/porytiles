#include "porytiles2/domain/packing/services/palette_packer.hpp"

#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Converts a \link PixelTile PixelTile's\endlink colors to a ColorSet.
 *
 * @details
 * Extracts unique non-transparent colors from the tile and looks up each color in the ColorIndexMap to build a ColorSet
 * bitset.
 *
 * @param tile The pixel tile to convert
 * @param color_map The color-to-index mapping
 * @param extrinsic The extrinsic transparency color
 * @pre All colors in the tile are present in color_map
 * @return ColorSet containing all non-transparent tile colors
 */
[[nodiscard]] ColorSet build_color_set_from_tile(
    const PixelTile<Rgba32> &tile, const ColorIndexMap<Rgba32> &color_map, const Rgba32 &extrinsic)
{
    ColorSet color_set{};
    const auto unique_colors = tile.unique_nontransparent_colors(extrinsic);

    for (const auto &color : unique_colors) {
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            panic("tile color " + to_string(color) + " not found in color index map");
        }
        color_set.set(index_opt.value());
    }

    return color_set;
}

/**
 * @brief Converts a fixed-size palette's non-wildcard colors to a ColorSet.
 *
 * @details
 * Extracts colors from slots 1-15 (skipping slot 0 which is transparency) and looks up each in the ColorIndexMap.
 * Wildcards are skipped.
 *
 * @param pal The palette to convert
 * @param color_map The color-to-index mapping
 * @pre All colors in the pal are present in color_map
 * @return ColorSet containing all non-transparent pal colors
 */
[[nodiscard]] ColorSet
build_color_set_from_pal(const Palette<Rgba32, pal::max_size> &pal, const ColorIndexMap<Rgba32> &color_map)
{
    ColorSet color_set{};

    // Start from slot 1 (slot 0 is transparency)
    for (std::size_t i = 1; i < pal.size(); ++i) {
        if (pal.is_wildcard(i)) {
            continue;
        }
        const auto color = pal.at(i);
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            panic("pal color " + to_string(color) + " at slot " + std::to_string(i) + " not in color map");
        }
        color_set.set(index_opt.value());
    }

    return color_set;
}

/**
 * @brief Converts a hint palette's colors to a ColorSet.
 *
 * @details
 * Extracts colors from the hint palette and looks up each in the ColorIndexMap. Wildcards are skipped.
 *
 * @param pal The dynamic palette to convert
 * @param color_map The color-to-index mapping
 * @pre All colors in the pal are present in color_map
 * @pre Palette contains no wildcards
 * @return ColorSet containing all non-transparent pal colors
 */
[[nodiscard]] ColorSet build_color_set_from_hint_pal(const Palette<Rgba32> &pal, const ColorIndexMap<Rgba32> &color_map)
{
    ColorSet color_set{};

    for (std::size_t i = 0; i < pal.size(); ++i) {
        if (pal.is_wildcard(i)) {
            panic("build_color_set_from_hint_palette palette contained unexpected wildcard");
        }
        const auto color = pal.at(i);
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            panic("hint color " + to_string(color) + " at slot " + std::to_string(i) + " not in color map");
        }
        color_set.set(index_opt.value());
    }

    return color_set;
}

/**
 * @brief Converts a PackedPalette back to a Palette<Rgba32, pal::max_size>.
 *
 * @details
 * Builds the output palette by:
 * 1. Preserving slot 0 from prefilled_pal (or using default_slot_zero if slot 0 was wildcarded or no prefilled_pal)
 * 2. Preserving non-wildcard slots from prefilled_pal
 * 3. Filling wildcard slots with colors from PackedPalette's ColorSet
 * 4. Filling remaining slots with Rgba32{0, 0, 0}
 *
 * @param packed_pal The packed palette result from the packer
 * @param prefilled_pal The original prefilled input palette (if any), or nullptr
 * @param color_map The color-to-index mapping for reverse lookup
 * @param default_slot_zero The default color for slot 0 if no prefilled input palette
 * @return The reconstructed Rgba32 palette
 */
[[nodiscard]] Palette<Rgba32, pal::max_size> build_output_palette(
    const PackedPalette &packed_pal,
    const Palette<Rgba32, pal::max_size> *prefilled_pal,
    const ColorIndexMap<Rgba32> &color_map,
    const Rgba32 &default_slot_zero)
{
    Palette<Rgba32, pal::max_size> output{Rgba32{0, 0, 0, Rgba32::alpha_opaque}};

    // Set slot 0 (transparency slot)
    if (prefilled_pal != nullptr && !prefilled_pal->is_wildcard(0)) {
        output.set(0, prefilled_pal->at(0));
    }
    else {
        output.set(0, default_slot_zero);
    }

    // Track which slots are filled from prefilled_pal and their colors
    std::set<std::size_t> filled_slots{};
    filled_slots.insert(0); // Slot 0 was filled above

    // Preserve non-wildcard slots from prefilled input palette

    // Collect colors from PackedPalette that still need to be placed

    // Place remaining colors in available slots
}

} // namespace

namespace porytiles2 {

ChainableResult<PalettePacking> PalettePacker::pack_tiles(const PackingParams &params) const
{
    // === STEP 1: Convert regular tiles to PackableTile vector ===
    std::vector<PackableTile> regular_tiles;
    regular_tiles.reserve(params.tiles_.size());
    for (std::size_t i = 0; i < params.tiles_.size(); ++i) {
        auto color_set = build_color_set_from_tile(params.tiles_[i], params.color_map_, params.extrinsic_transparency_);
        regular_tiles.emplace_back(PackableTile::RegularId{i}, color_set);
    }

    // === STEP 2: Convert hints to PackableTile vector ===
    std::vector<PackableTile> hint_tiles;
    hint_tiles.reserve(params.hints_.size());
    for (const PaletteHint &hint : params.hints_) {
        auto color_set = build_color_set_from_hint_pal(hint.pal(), params.color_map_);
        hint_tiles.emplace_back(PackableTile::HintId{hint.name()}, color_set);
    }

    // === STEP 3: Convert input prefilled palettes to PrefilledPalette vector ===
    std::set<PrefilledPalette> prefilled_palettes;
    for (std::size_t i = 0; i < params.prefilled_pals_.size(); ++i) {
        if (!params.prefilled_pals_[i].has_value()) {
            continue;
        }
        ColorSet color_set = build_color_set_from_pal(params.prefilled_pals_[i].value(), params.color_map_);
        prefilled_palettes.insert(PrefilledPalette::partially_locked(i, color_set));
    }

    // === STEP 4: Create PackingInput and call low-level pack() ===
    // TODO: set up available_pals_ bitset properly
    PackingInput packing_input{
        std::move(regular_tiles), std::move(hint_tiles), std::move(prefilled_palettes), std::bitset<pal::num_pals>{}};

    PT_TRY_ASSIGN_CHAIN_ERR(
        packing_output, strategy_->pack(packing_input), "low-level palette packing failed", PalettePacking);

    // === STEP 5: Convert PackingOutput back to PalettePacking ===
    PalettePacking packing{};
    // TODO: figure out these statements
    // packing.tile_to_pal_ = packing_output.tile_to_pal_;
    // packing.pals_.reserve(packing_output.pals_.size());

    for (const PackedPalette &packed_pal : packing_output.pals_) {
        const std::size_t hardware_index = packed_pal.hardware_index();
        if (hardware_index >= pal::num_pals) {
            panic("invalid hardware index " + std::to_string(hardware_index) + ": out of range");
        }
        // TODO: use build_output_palette to turn packed_pal into the output pal at hw_index
    }

    return packing;
}

} // namespace porytiles2

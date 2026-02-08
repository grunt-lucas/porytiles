#include "porytiles2/domain/packing/services/palette_packer.hpp"

#include "porytiles2/domain/packing/models/palette_pool.hpp"
#include "porytiles2/utilities/panic/panic.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace {

using namespace porytiles2;

/**
 * @brief Result of extracting colors from a palette, tracking both unique colors and occupied slots.
 */
struct ColorSetWithOccupancy {
    ColorSet color_set;
    std::size_t occupied_slots{};
};

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
            /*
             * This will throw if a hint contains the extrinsic transparency color, since the ColorIndexMap won't
             * contain any transparency colors. Callers of the packer service should have used the PaletteValidator
             * service to validate input palettes and generate good user diagnostics.
             */
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
 * Wildcards are skipped. Returns both the ColorSet of unique colors and the count of occupied slots.
 *
 * @param pal The palette to convert
 * @param color_map The color-to-index mapping
 * @pre All colors in the pal are present in color_map
 * @return ColorSetWithOccupancy containing unique colors and occupied slot count
 */
[[nodiscard]] ColorSetWithOccupancy
build_color_set_from_pal(const Palette<Rgba32, pal::max_size> &pal, const ColorIndexMap<Rgba32> &color_map)
{
    ColorSet color_set{};
    std::size_t occupied_slots = 0;

    // Start from slot 1 (slot 0 is transparency)
    for (std::size_t i = 1; i < pal.size(); ++i) {
        if (pal.is_wildcard(i)) {
            continue;
        }
        ++occupied_slots;
        const auto color = pal.at(i);
        const auto index_opt = color_map.index_at_color(color);
        if (!index_opt.has_value()) {
            /*
             * This will throw if a pal contains the extrinsic transparency color, since the ColorIndexMap won't
             * contain any transparency colors. Callers of the packer service should have used the PaletteValidator
             * service to validate input palettes and generate good user diagnostics.
             */
            panic("pal color " + to_string(color) + " at slot " + std::to_string(i) + " not in color map");
        }
        color_set.set(index_opt.value());
    }

    return ColorSetWithOccupancy{color_set, occupied_slots};
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
    /*
     * TODO: due to this construction, unused wildcard slots will get 0,0,0. Perhaps this should be configurable? If we
     * default construct the output palette, then any unused slots in any of the palettes (e.g. unsued slots in the
     * regular packed pals, wildcard slots in the prefilled pals), will be wildcarded in the returned pals. It would be
     * up to the calling code to fix this. Let's think carefully about whose responsibility it should be to fill in
     * wildcard slots. The packer, or the one calling the packer.
     */
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
    if (prefilled_pal != nullptr) {
        for (std::size_t i = 1; i < pal::max_size; ++i) {
            if (!prefilled_pal->is_wildcard(i)) {
                output.set(i, prefilled_pal->at(i));
                filled_slots.insert(i);
            }
        }
    }

    // Collect colors from PackedPalette that still need to be placed
    std::vector<Rgba32> rgba32s_to_place;
    for_each_color(packed_pal.color_set(), [&](const std::size_t color_index) {
        const auto color_opt = color_map.color_at_index(ColorIndex{color_index});
        if (!color_opt.has_value()) {
            panic("color_index " + std::to_string(color_index) + " not found in color map");
        }
        const auto &color = color_opt.value();
        // Check if this color is already placed
        bool already_placed = false;
        for (const std::size_t slot : filled_slots) {
            if (output.at(slot) == color) {
                already_placed = true;
                break;
            }
        }
        if (!already_placed) {
            rgba32s_to_place.push_back(color);
        }
    });

    /*
     * TODO: this is the location where we'll eventually want to implement the logic for:
     *
     *   tile_sharing: opportunistic
     *
     * wherein we attempt to line up colors so that tiles can be shared. At that point, we'll probably want to have some
     * kind of OutputPaletteBuilder service instead of just a single method.
     */

    // Place remaining colors in available slots
    std::size_t next_slot = 1;
    std::size_t placed_count = 0;
    for (const auto &rgba : rgba32s_to_place) {
        while (next_slot < pal::max_size && filled_slots.contains(next_slot)) {
            ++next_slot;
        }
        if (next_slot < pal::max_size) {
            output.set(next_slot, rgba);
            filled_slots.insert(next_slot);
            ++next_slot;
            ++placed_count;
        }
    }

    if (placed_count != rgba32s_to_place.size()) {
        panic("failed to place all colors in rgba32s_to_place");
    }

    return output;
}

} // namespace

namespace porytiles2 {

ChainableResult<PalettePacking> PalettePacker::pack_tiles(const PackingParams &params) const
{
    // === STEP 1: Convert regular tiles and anims to PackableTile vector ===
    std::vector<PackableTile> regular_tiles;
    regular_tiles.reserve(params.tiles_.size());
    for (std::size_t i = 0; i < params.tiles_.size(); ++i) {
        auto color_set = build_color_set_from_tile(params.tiles_[i], params.color_map_, params.extrinsic_transparency_);
        regular_tiles.emplace_back(PackableTile::RegularId{i}, color_set);
    }
    for (const auto &[anim_name, anim] : params.anims_) {
        const auto &composite_frame = anim.composite_frame(params.extrinsic_transparency_);
        for (std::size_t subtile_index = 0; subtile_index < composite_frame.tiles().size(); ++subtile_index) {
            const auto &composite_tile = composite_frame.tiles().at(subtile_index);
            auto color_set =
                build_color_set_from_tile(composite_tile, params.color_map_, params.extrinsic_transparency_);
            regular_tiles.emplace_back(PackableTile::AnimId{anim_name, subtile_index}, color_set);
        }
    }

    // === STEP 2: Convert hints to PackableTile vector ===
    std::vector<PackableTile> hint_tiles;
    hint_tiles.reserve(params.hints_.size());
    for (const PaletteHint &hint : params.hints_) {
        auto color_set = build_color_set_from_hint_pal(hint.pal(), params.color_map_);
        hint_tiles.emplace_back(PackableTile::HintId{hint.name()}, color_set);
    }

    // === STEP 3: Convert input prefilled palettes to PrefilledPalette set ===
    std::set<PrefilledPalette> prefilled_palettes;
    for (std::size_t i = 0; i < params.prefilled_pals_.size(); ++i) {
        if (!params.prefilled_pals_[i].has_value()) {
            continue;
        }
        auto [color_set, occupied_slots] =
            build_color_set_from_pal(params.prefilled_pals_[i].value(), params.color_map_);
        prefilled_palettes.insert(PrefilledPalette::partially_locked(i, color_set, occupied_slots));
    }

    // === STEP 4: Create PackingInput and call low-level pack() ===
    PackingInput packing_input{
        std::move(regular_tiles),
        std::move(hint_tiles),
        std::move(prefilled_palettes),
        PalettePool{params.available_pals_}};

    PT_TRY_ASSIGN_CHAIN_ERR(
        packing_output, strategy_->pack(packing_input), "Low-level palette packing failed.", PalettePacking);

    // === STEP 5: Convert PackingOutput back to PalettePacking ===
    PalettePacking packing{};

    // 5a: build the tile_to_pal maps for PalettePacking
    for (const auto &[tile_id, pal_index] : packing_output.tile_to_pal_) {
        std::visit(
            [&packing, pal_index]<typename IdVariant>(IdVariant &&id) {
                using Id = std::decay_t<IdVariant>;
                if constexpr (std::is_same_v<Id, PackableTile::RegularId>) {
                    // For regular tiles, update the tile index to pal index map
                    packing.tile_to_pal_[id.index] = pal_index;
                }
                else if constexpr (std::is_same_v<Id, PackableTile::AnimId>) {
                    // TODO: ANIM: do we care about anim?
                }
                else if constexpr (std::is_same_v<Id, PackableTile::HintId>) {
                    // We don't currently care to store where hints got assigned
                }
                else if constexpr (std::is_same_v<Id, PackableTile::PrefilledPaletteId>) {
                    // Nothing to do here, we only had these PackableTiles for internal bookkeeping
                }
                else {
                    panic("unimplemented std::visit branch");
                }
            },
            tile_id);
    }

    // 5b: build the pals array for PalettePacking
    for (const PackedPalette &packed_pal : packing_output.pals_) {
        const std::size_t hardware_index = packed_pal.hardware_index();
        if (hardware_index >= pal::num_pals) {
            panic("invalid hardware index " + std::to_string(hardware_index) + ": out of range");
        }
        const Palette<Rgba32, pal::max_size> *input_pal_ptr = params.prefilled_pals_[hardware_index].has_value()
                                                                  ? &params.prefilled_pals_[hardware_index].value()
                                                                  : nullptr;
        packing.pals_[hardware_index] =
            build_output_palette(packed_pal, input_pal_ptr, params.color_map_, params.extrinsic_transparency_);
    }

    /*
     * TODO: above, we built output pals from all the pals that got explicitly packed. But what about pals that got
     * filled via Porytiles pal overrides? We need to think about this very carefully. We'll need to think through how
     * we populate the bitset for the PalettePool above.
     *
     * Here's what I think we should do. PalettePacker should only handle palettes that were explicitly enabled for
     * packing via PalettePool. Any palette not turned on in PalettePool will get sent back to the caller as a
     * std::nullopt. The caller can then decide what to do (e.g. fill in with Porytiles override, fill in with default
     * values, etc).
     *
     * Callers MUST explicitly enable all pals they want the packer to access. This means that if the caller is
     * compiling a secondary set, they should enable bits 0-5 in the available_pals_ bitset in PackingParams, and then
     * pass the packer the primary palettes via the prefilled_pals_ input param. The prefilled 0-5 will have no
     * wildcards since they are completely fixed by the primary. The calling code should also probably throw a warning
     * if the user specified Porytiles overrides for those pals since they will be ignored in favor of the palettes for
     * paired primary (maybe this can be configurable?). The calling code should also throw a warning when there are
     * Porytiles overrides that don't correspond to one of the pals enabled for packing. E.g. if we are compiling
     * primary, so bits 0-5 are set in the PalettePool, but user has supplied 12.pal in the Porytiles palettes, warn
     * user that they've supplied an out-of-band palette due to configuration. Then say that this palette will still be
     * copied over and all wildcards will receive default values (or allow user to specify an alternate behavior via
     * configuration).
     */

    return packing;
}

} // namespace porytiles2

#include "porytiles/infra/services/base_game_detector.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace porytiles {

ChainableResult<BaseGame> BaseGameDetector::detect() const
{
    const std::filesystem::path global_fieldmap_path = project_root_ / "include" / "global.fieldmap.h";

    if (!std::filesystem::exists(global_fieldmap_path)) {
        return FormattableError{
            "Base game detection failed: '{}' does not exist.",
            FormatParam{global_fieldmap_path.string(), Style::bold}};
    }

    /*
     * Simple text search for base-game-specific markers. We avoid CParserFacade::find_define() because it attempts
     * to parse ALL defines in the file, including complex multiline macros that may fail to parse (e.g.,
     * backslash-continuation lines).
     *
     * Decision tree based on 4 boolean markers:
     *   METATILE_ATTRIBUTE_BEHAVIOR       -> pokefirered or pokeemerald-expansion (enum-based attributes)
     *     + METATILE_ATTR_BEHAVIOR_MASK   -> pokeemerald-expansion (has both enum and #define attribute markers)
     *     - METATILE_ATTR_BEHAVIOR_MASK   -> pokefirered
     *   METATILE_ATTR_BEHAVIOR_MASK       -> emerald-family (#define-based)
     *     + MAPGRID_METATILE_ID_SHIFT     -> emerald or expansion
     *       + swapPalettes               -> pokeemerald-expansion
     *       - swapPalettes               -> pokeemerald
     *     - MAPGRID_METATILE_ID_SHIFT     -> pokeruby
     */
    std::ifstream file{global_fieldmap_path};
    if (!file) {
        return FormattableError{
            "Failed to open '{}' for base game detection.", FormatParam{global_fieldmap_path.string(), Style::bold}};
    }

    bool has_metatile_attribute_behavior = false;
    bool has_metatile_attr_behavior_mask = false;
    bool has_mapgrid_metatile_id_shift = false;
    bool has_swap_palettes = false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("METATILE_ATTRIBUTE_BEHAVIOR") != std::string::npos) {
            has_metatile_attribute_behavior = true;
        }
        if (line.find("METATILE_ATTR_BEHAVIOR_MASK") != std::string::npos) {
            has_metatile_attr_behavior_mask = true;
        }
        if (line.find("MAPGRID_METATILE_ID_SHIFT") != std::string::npos) {
            has_mapgrid_metatile_id_shift = true;
        }
        if (line.find("swapPalettes") != std::string::npos) {
            has_swap_palettes = true;
        }
    }

    BaseGame detected;
    std::string reason;

    if (has_metatile_attribute_behavior) {
        if (has_metatile_attr_behavior_mask) {
            detected = BaseGame::pokeemerald_expansion;
            reason = "Found both METATILE_ATTRIBUTE_BEHAVIOR and METATILE_ATTR_BEHAVIOR_MASK";
        }
        else {
            detected = BaseGame::pokefirered;
            reason = "Found METATILE_ATTRIBUTE_BEHAVIOR";
        }
    }
    else if (has_metatile_attr_behavior_mask) {
        if (has_mapgrid_metatile_id_shift) {
            if (has_swap_palettes) {
                detected = BaseGame::pokeemerald_expansion;
                reason = "Found METATILE_ATTR_BEHAVIOR_MASK, MAPGRID_METATILE_ID_SHIFT, and swapPalettes";
            }
            else {
                detected = BaseGame::pokeemerald;
                reason = "Found METATILE_ATTR_BEHAVIOR_MASK and MAPGRID_METATILE_ID_SHIFT";
            }
        }
        else {
            detected = BaseGame::pokeruby;
            reason = "Found METATILE_ATTR_BEHAVIOR_MASK but no MAPGRID_METATILE_ID_SHIFT";
        }
    }
    else {
        return FormattableError{
            "Base game detection failed: no recognized markers found in '{}'.",
            FormatParam{global_fieldmap_path.string(), Style::bold}};
    }

    constexpr auto tag = "base-game-detection";
    diag_->remark(tag, format_->format("Detected base game '{}'.", FormatParam{to_string(detected), Style::bold}));
    diag_->remark_note(
        tag,
        format_->format("{} in '{}'.", FormatParam{reason}, FormatParam{global_fieldmap_path.string(), Style::bold}));

    return detected;
}

} // namespace porytiles

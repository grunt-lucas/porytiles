#pragma once

#include <filesystem>

#include "gsl/pointers"

#include "porytiles2/domain/models/base_game.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Detects the base game of a decompilation project.
 *
 * @details
 * Inspects the project's @c include/global.fieldmap.h header to determine which of the four supported base games the
 * project targets: pokeemerald, pokefirered, pokeruby, or pokeemerald-expansion. Detection is based on content
 * heuristics within the header file:
 *
 * - @c METATILE_ATTRIBUTE_BEHAVIOR (enum-based) → pokefirered
 * - @c METATILE_ATTR_BEHAVIOR_MASK + @c MAPGRID_METATILE_ID_SHIFT + @c swapPalettes → pokeemerald-expansion
 * - @c METATILE_ATTR_BEHAVIOR_MASK + @c MAPGRID_METATILE_ID_SHIFT → pokeemerald
 * - @c METATILE_ATTR_BEHAVIOR_MASK only → pokeruby
 *
 * Emits a diagnostic remark with tag @c "base-game-detection" on successful detection.
 */
class BaseGameDetector {
  public:
    BaseGameDetector(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
    {
    }

    /**
     * @brief Detects the base game for the project.
     *
     * @details
     * Reads @c include/global.fieldmap.h and scans for content markers that uniquely identify each supported base
     * game. Returns an error if the file is missing, unreadable, or contains no recognized markers.
     *
     * @return The detected BaseGame, or an error if detection fails
     */
    [[nodiscard]] ChainableResult<BaseGame> detect() const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2

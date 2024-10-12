#ifndef PORYTILES_PALETTE_ASSIGNMENT_H
#define PORYTILES_PALETTE_ASSIGNMENT_H

#include <cstddef>
#include <vector>

#include "compiler.h"
#include "porytiles_context.h"
#include "types.h"

namespace porytiles {

constexpr std::size_t EXPLORATION_CUTOFF_MULTIPLIER = 1'000'000;
constexpr std::size_t EXPLORATION_MAX_CUTOFF = 100 * EXPLORATION_CUTOFF_MULTIPLIER;

struct AssignState {
  /*
   * One color set for each hardware palette, bits in color set will indicate which colors this HW palette will have.
   * The size of the vector should be fixed to maxPalettes.
   */
  std::vector<ColorSet> hardwarePalettes;

  // The count of unassigned palettes
  std::size_t unassignedCount;

  // The count of unassigned primer palettes
  std::size_t unassignedPrimerCount;

  auto operator==(const AssignState &other) const
  {
    return this->hardwarePalettes == other.hardwarePalettes && this->unassignedCount == other.unassignedCount &&
           this->unassignedPrimerCount == other.unassignedPrimerCount;
  }
};

enum class AssignResult { SUCCESS, EXPLORE_CUTOFF_REACHED, NO_SOLUTION_POSSIBLE };
} // namespace porytiles

template <> struct std::hash<porytiles::AssignState> {
  std::size_t operator()(const porytiles::AssignState &state) const noexcept
  {
    std::size_t hashValue = 0;
    for (auto bitset : state.hardwarePalettes) {
      hashValue ^= std::hash<std::bitset<240>>{}(bitset);
    }
    hashValue ^= std::hash<std::bitset<240>>{}(state.unassignedCount);
    return hashValue;
  }
};

namespace porytiles {
AssignResult assignDepthFirst(PorytilesContext &ctx, CompilerMode compilerMode, AssignState &state,
                              std::vector<ColorSet> &solution, const std::vector<ColorSet> &primaryPalettes,
                              const std::vector<ColorSet> &unassigneds, const std::vector<ColorSet> &unassignedPrimers);
AssignResult assignBreadthFirst(PorytilesContext &ctx, CompilerMode compilerMode, AssignState &initialState,
                                std::vector<ColorSet> &solution, const std::vector<ColorSet> &primaryPalettes,
                                const std::vector<ColorSet> &unassigneds,
                                const std::vector<ColorSet> &unassignedPrimers);
std::pair<std::vector<ColorSet>, std::vector<ColorSet>>
runPaletteAssignmentMatrix(PorytilesContext &ctx, CompilerMode compilerMode, const std::vector<ColorSet> &colorSets,
                           const std::vector<ColorSet> &primerColorSets,
                           const std::unordered_map<BGR15, std::size_t> &colorToIndex);
} // namespace porytiles

#endif // PORYTILES_PALETTE_ASSIGNMENT_H
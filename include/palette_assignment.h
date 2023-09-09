#ifndef PORYTILES_PALETTE_ASSIGNMENT_H
#define PORYTILES_PALETTE_ASSIGNMENT_H

#include <cstddef>
#include <vector>

#include "compiler.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

constexpr std::size_t EXPLORATION_CUTOFF_MULTIPLIER = 1'000'000;

struct AssignState {
  /*
   * One color set for each hardware palette, bits in color set will indicate which colors this HW palette will have.
   * The size of the vector should be fixed to maxPalettes.
   */
  std::vector<ColorSet> hardwarePalettes;

  // The count of unassigned palettes
  std::size_t unassignedCount;

  auto operator==(const AssignState &other) const
  {
    return this->hardwarePalettes == other.hardwarePalettes && this->unassignedCount == other.unassignedCount;
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
AssignResult assignDepthFirst(PtContext &ctx, AssignState &state, std::vector<ColorSet> &solution,
                              const std::vector<ColorSet> &primaryPalettes, const std::vector<ColorSet> &unassigneds);
AssignResult assignBreadthFirst(PtContext &ctx, AssignState &initialState, std::vector<ColorSet> &solution,
                                const std::vector<ColorSet> &primaryPalettes, const std::vector<ColorSet> &unassigneds);
} // namespace porytiles

#endif // PORYTILES_PALETTE_ASSIGNMENT_H
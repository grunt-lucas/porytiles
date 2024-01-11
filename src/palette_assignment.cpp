#include "palette_assignment.h"

#include <algorithm>
#include <deque>
#include <unordered_set>
#include <vector>

#include "logger.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {
AssignResult assignDepthFirst(PtContext &ctx, AssignState &state, std::vector<ColorSet> &solution,
                              const std::vector<ColorSet> &primaryPalettes, const std::vector<ColorSet> &unassigneds,
                              const std::vector<ColorSet> &unassignedPrimers)
{
  std::size_t exploredNodeCutoff = ctx.compilerConfig.mode == CompilerMode::PRIMARY
                                       ? ctx.compilerConfig.primaryExploredNodeCutoff
                                       : ctx.compilerConfig.secondaryExploredNodeCutoff;
  std::size_t bestBranches = ctx.compilerConfig.mode == CompilerMode::PRIMARY
                                 ? ctx.compilerConfig.primaryBestBranches
                                 : ctx.compilerConfig.secondaryBestBranches;
  bool smartPrune = ctx.compilerConfig.mode == CompilerMode::PRIMARY ? ctx.compilerConfig.primarySmartPrune
                                                                     : ctx.compilerConfig.secondarySmartPrune;

  ctx.compilerContext.exploredNodeCounter++;
  if (ctx.compilerContext.exploredNodeCounter % EXPLORATION_CUTOFF_MULTIPLIER == 0) {
    pt_logln(ctx, stderr, "exploredNodeCounter passed {} iterations", ctx.compilerContext.exploredNodeCounter);
  }
  if (ctx.compilerContext.exploredNodeCounter > exploredNodeCutoff) {
    return AssignResult::EXPLORE_CUTOFF_REACHED;
  }

  // TODO : add state.unassignedPrimersCount == 0
  if (state.unassignedCount == 0) {
    // No tiles left to assign, found a solution!
    std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes), std::back_inserter(solution));
    return AssignResult::SUCCESS;
  }

  /*
   * We will try to assign the last element to one of the 6 hw palettes, last because it is a vector so easier to
   * add/remove from the end. First we assign all the primer palettes, then we assign the regular palettes.
   */
  const ColorSet &toAssign = unassigneds.at(state.unassignedCount - 1);
  // TODO : fill this code
  // if (!unassignedPrimers.empty()) {

  // }
  // else if (unassignedPrimers.empty() && !unassigneds.empty()) {

  // }

  /*
   * If we are assigning a secondary set, we'll want to first check if any of the primary palettes satisfy the color
   * constraints for this particular tile. That way we can just use the primary palette, since those are available for
   * secondary tiles to freely use.
   */
  if (!primaryPalettes.empty()) {
    for (std::size_t i = 0; i < primaryPalettes.size(); i++) {
      const ColorSet &palette = primaryPalettes.at(i);
      if ((palette | toAssign).count() == palette.count()) {
        /*
         * This case triggers if `toAssign' shares all its colors with one of the palettes from the primary
         * tileset. In that case, we will just reuse that palette when we make the tile in a later step. So we
         * can prep a recursive call to assign with an unchanged state (other than removing `toAssign')
         */
        std::vector<ColorSet> hardwarePalettesCopy;
        std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
                  std::back_inserter(hardwarePalettesCopy));
        AssignState updatedState = {hardwarePalettesCopy, state.unassignedCount - 1};

        AssignResult result =
            assignDepthFirst(ctx, updatedState, solution, primaryPalettes, unassigneds, unassignedPrimers);
        if (result == AssignResult::SUCCESS) {
          return AssignResult::SUCCESS;
        }
        else if (result == AssignResult::EXPLORE_CUTOFF_REACHED) {
          return AssignResult::EXPLORE_CUTOFF_REACHED;
        }
      }
    }
  }

  /*
   * For this next step, we want to sort the hw palettes before we try iterating. Sort them by the size of their
   * intersection with the toAssign ColorSet. Effectively, this means that we will always first try branching into an
   * assignment that re-uses hw palettes more effectively. We also have a tie-breaker heuristic for cases where two
   * palettes have the same intersect size. Right now we just use palette size, but in the future we may want to look
   * at color distances so we can pick a palette with more similar colors.
   */
  std::stable_sort(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
                   [&toAssign](const auto &pal1, const auto &pal2) {
                     std::size_t pal1IntersectSize = (pal1 & toAssign).count();
                     std::size_t pal2IntersectSize = (pal2 & toAssign).count();

                     /*
                      * FEATURE : Instead of just using palette count, maybe can we check for color distance here and
                      * try to choose the palette that has the "closest" colors to our toAssign palette? That might be a
                      * good heuristic for attempting to keep similar colors in the same palette. I.e. especially in
                      * cases where there are no palette intersections, it may be better to first try placing the new
                      * colors into a palette with similar colors rather than into the smallest palette. We can put
                      * this behind a flag like '-Ocolor-distance-heuristic
                      */
                     if (pal1IntersectSize == pal2IntersectSize) {
                       return pal1.count() < pal2.count();
                     }

                     return pal1IntersectSize > pal2IntersectSize;
                   });

  std::size_t stopLimit = std::min(state.hardwarePalettes.size(), bestBranches);
  if (smartPrune) {
    throw std::runtime_error{"TODO : impl smart prune"};
  }
  for (size_t i = 0; i < stopLimit; i++) {
    const ColorSet &palette = state.hardwarePalettes.at(i);

    // > PAL_SIZE - 1 because we need to save a slot for transparency
    if ((palette | toAssign).count() > PAL_SIZE - 1) {
      /*
       *  Skip this palette, cannot assign because there is not enough room in the palette. If we end up skipping
       * all of them that means the palettes are all too full and we cannot assign this tile in the state we are
       * in. The algorithm will be forced to backtrack and try other assignments.
       */
      continue;
    }

    /*
     * Prep the recursive call to assign(). If we got here, we know it is possible to assign toAssign to the palette
     * at hardwarePalettes[i]. So we make a copy of unassigned with toAssign removed and a copy of hardwarePalettes
     * with toAssigned assigned to the palette at index i. Then we call assign again with this updated state, and
     * return true if there is a valid solution somewhere down in this recursive branch.
     */
    std::vector<ColorSet> hardwarePalettesCopy;
    std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
              std::back_inserter(hardwarePalettesCopy));
    hardwarePalettesCopy.at(i) |= toAssign;
    AssignState updatedState = {hardwarePalettesCopy, state.unassignedCount - 1};

    AssignResult result =
        assignDepthFirst(ctx, updatedState, solution, primaryPalettes, unassigneds, unassignedPrimers);
    if (result == AssignResult::SUCCESS) {
      return AssignResult::SUCCESS;
    }
    else if (result == AssignResult::EXPLORE_CUTOFF_REACHED) {
      return AssignResult::EXPLORE_CUTOFF_REACHED;
    }
  }

  // No solution found
  return AssignResult::NO_SOLUTION_POSSIBLE;
}

AssignResult assignBreadthFirst(PtContext &ctx, AssignState &initialState, std::vector<ColorSet> &solution,
                                const std::vector<ColorSet> &primaryPalettes, const std::vector<ColorSet> &unassigneds,
                                const std::vector<ColorSet> &unassignedPrimers)
{
  std::size_t exploredNodeCutoff = ctx.compilerConfig.mode == CompilerMode::PRIMARY
                                       ? ctx.compilerConfig.primaryExploredNodeCutoff
                                       : ctx.compilerConfig.secondaryExploredNodeCutoff;
  std::size_t bestBranches = ctx.compilerConfig.mode == CompilerMode::PRIMARY
                                 ? ctx.compilerConfig.primaryBestBranches
                                 : ctx.compilerConfig.secondaryBestBranches;
  bool smartPrune = ctx.compilerConfig.mode == CompilerMode::PRIMARY ? ctx.compilerConfig.primarySmartPrune
                                                                     : ctx.compilerConfig.secondarySmartPrune;

  std::unordered_set<AssignState> visitedStates{};
  std::deque<AssignState> stateQueue{};
  std::deque<AssignState> lowPriorityQueue{};
  stateQueue.push_back(initialState);
  visitedStates.insert(initialState);

  while (!stateQueue.empty() || !lowPriorityQueue.empty()) {
    AssignState currentState;
    ctx.compilerContext.exploredNodeCounter++;
    if (ctx.compilerContext.exploredNodeCounter % EXPLORATION_CUTOFF_MULTIPLIER == 0) {
      pt_logln(ctx, stderr, "exploredNodeCounter passed factor {}, stateQueue={}, lowPrioQueue={}",
               ctx.compilerContext.exploredNodeCounter / EXPLORATION_CUTOFF_MULTIPLIER, stateQueue.size(),
               lowPriorityQueue.size());
    }
    if (ctx.compilerContext.exploredNodeCounter > exploredNodeCutoff) {
      return AssignResult::EXPLORE_CUTOFF_REACHED;
    }

    if (!stateQueue.empty()) {
      currentState = stateQueue.front();
      stateQueue.pop_front();
    }
    else if (!lowPriorityQueue.empty()) {
      currentState = lowPriorityQueue.front();
      lowPriorityQueue.pop_front();
    }

    if (currentState.unassignedCount == 0) {
      // No tiles left to assign, found a solution!
      std::copy(std::begin(currentState.hardwarePalettes), std::end(currentState.hardwarePalettes),
                std::back_inserter(solution));
      return AssignResult::SUCCESS;
    }

    const ColorSet &toAssign = unassigneds.at(currentState.unassignedCount - 1);

    bool foundPrimaryMatch = false;
    if (!primaryPalettes.empty()) {
      for (std::size_t i = 0; i < primaryPalettes.size(); i++) {
        const ColorSet &palette = primaryPalettes.at(i);
        if ((palette | toAssign).count() == palette.count()) {
          std::vector<ColorSet> hardwarePalettesCopy;
          std::copy(std::begin(currentState.hardwarePalettes), std::end(currentState.hardwarePalettes),
                    std::back_inserter(hardwarePalettesCopy));
          AssignState updatedState = {hardwarePalettesCopy, currentState.unassignedCount - 1};
          stateQueue.push_back(updatedState);
          visitedStates.insert(updatedState);
          foundPrimaryMatch = true;
        }
      }
    }

    /*
     * If we found a matching primary palette for the current assignment, go ahead and skip ahead to the next toAssign.
     * No need to process anything further for this toAssign.
     */
    if (foundPrimaryMatch) {
      continue;
    }

    std::stable_sort(std::begin(currentState.hardwarePalettes), std::end(currentState.hardwarePalettes),
                     [&toAssign](const auto &pal1, const auto &pal2) {
                       std::size_t pal1IntersectSize = (pal1 & toAssign).count();
                       std::size_t pal2IntersectSize = (pal2 & toAssign).count();
                       if (pal1IntersectSize == pal2IntersectSize) {
                         return pal1.count() < pal2.count();
                       }
                       return pal1IntersectSize > pal2IntersectSize;
                     });

    bool sawAssignmentWithIntersection = false;
    std::size_t stopLimit = std::min(currentState.hardwarePalettes.size(), bestBranches);
    if (smartPrune) {
      // TODO : impl smart prune feature
      throw std::runtime_error{"TODO : impl smart prune"};
    }
    for (size_t i = 0; i < stopLimit; i++) {
      const ColorSet &palette = currentState.hardwarePalettes.at(i);

      // > PAL_SIZE - 1 because we need to save a slot for transparency
      if ((palette | toAssign).count() > PAL_SIZE - 1) {
        continue;
      }

      if ((palette & toAssign).count() > 0) {
        sawAssignmentWithIntersection = true;
      }

      std::vector<ColorSet> hardwarePalettesCopy;
      std::copy(std::begin(currentState.hardwarePalettes), std::end(currentState.hardwarePalettes),
                std::back_inserter(hardwarePalettesCopy));
      hardwarePalettesCopy.at(i) |= toAssign;
      AssignState updatedState = {hardwarePalettesCopy, currentState.unassignedCount - 1};
      if (!visitedStates.contains(updatedState)) {
        if (sawAssignmentWithIntersection && (palette & toAssign).count() == 0) {
          /*
           * Heuristic: if we already saw at least one assignment that had some intersection, put the 0-intersection
           * branches in a lower-priority queue
           */
          lowPriorityQueue.push_back(updatedState);
          visitedStates.insert(updatedState);
        }
        else {
          stateQueue.push_back(updatedState);
          visitedStates.insert(updatedState);
        }
      }
    }
  }

  return AssignResult::NO_SOLUTION_POSSIBLE;
}

// std::pair<std::vector<ColorSet>, std::vector<ColorSet>>
static auto tryAssignment(PtContext &ctx, const std::vector<ColorSet> &colorSets,
                          const std::vector<ColorSet> &primerColorSets,
                          const std::unordered_map<BGR15, std::size_t> &colorToIndex, bool printErrors)
{
  std::vector<ColorSet> assignedPalsSolution{};
  std::vector<ColorSet> tmpHardwarePalettes{};
  if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
    assignedPalsSolution.reserve(ctx.fieldmapConfig.numPalettesInPrimary);
    tmpHardwarePalettes.resize(ctx.fieldmapConfig.numPalettesInPrimary);
  }
  else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    assignedPalsSolution.reserve(ctx.fieldmapConfig.numPalettesInSecondary());
    tmpHardwarePalettes.resize(ctx.fieldmapConfig.numPalettesInSecondary());
  }
  else {
    internalerror_unknownCompilerMode("compiler::compile");
  }
  std::vector<ColorSet> unassignedNormPalettes;
  std::vector<ColorSet> unassignedPrimerPalettes;
  std::copy(std::begin(colorSets), std::end(colorSets), std::back_inserter(unassignedNormPalettes));
  std::copy(std::begin(primerColorSets), std::end(primerColorSets), std::back_inserter(unassignedPrimerPalettes));
  std::stable_sort(std::begin(unassignedNormPalettes), std::end(unassignedNormPalettes),
                   [](const auto &cs1, const auto &cs2) { return cs1.count() < cs2.count(); });
  std::stable_sort(std::begin(unassignedPrimerPalettes), std::end(unassignedPrimerPalettes),
                   [](const auto &cs1, const auto &cs2) { return cs1.count() < cs2.count(); });
  std::vector<ColorSet> primaryPaletteColorSets{};
  if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
    /*
     * Construct ColorSets for the primary palettes, assign can use these to decide if a tile is entirely covered by a
     * primary palette and hence does not need to extend the search by assigning its colors to one of the new secondary
     * palettes.
     */
    primaryPaletteColorSets.reserve(ctx.compilerContext.pairedPrimaryTileset->palettes.size());
    for (std::size_t i = 0; i < ctx.compilerContext.pairedPrimaryTileset->palettes.size(); i++) {
      const auto &gbaPalette = ctx.compilerContext.pairedPrimaryTileset->palettes.at(i);
      primaryPaletteColorSets.emplace_back();
      for (std::size_t j = 1; j < gbaPalette.size; j++) {
        primaryPaletteColorSets.at(i).set(colorToIndex.at(gbaPalette.colors.at(j)));
      }
    }
  }

  AssignState initialState = {tmpHardwarePalettes, unassignedNormPalettes.size()};
  ctx.compilerContext.exploredNodeCounter = 0;
  AssignResult assignResult = AssignResult::NO_SOLUTION_POSSIBLE;
  AssignAlgorithm assignAlgorithm = ctx.compilerConfig.mode == CompilerMode::PRIMARY
                                        ? ctx.compilerConfig.primaryAssignAlgorithm
                                        : ctx.compilerConfig.secondaryAssignAlgorithm;
  std::size_t exploredNodeCutoff = ctx.compilerConfig.mode == CompilerMode::PRIMARY
                                       ? ctx.compilerConfig.primaryExploredNodeCutoff
                                       : ctx.compilerConfig.secondaryExploredNodeCutoff;
  if (assignAlgorithm == AssignAlgorithm::DFS) {
    assignResult = assignDepthFirst(ctx, initialState, assignedPalsSolution, primaryPaletteColorSets,
                                    unassignedNormPalettes, unassignedPrimerPalettes);
  }
  else if (assignAlgorithm == AssignAlgorithm::BFS) {
    assignResult = assignBreadthFirst(ctx, initialState, assignedPalsSolution, primaryPaletteColorSets,
                                      unassignedNormPalettes, unassignedPrimerPalettes);
  }
  else {
    internalerror("palette_assignment::tryAssignment unknown AssignAlgorithm");
  }

  if (assignResult == AssignResult::NO_SOLUTION_POSSIBLE) {
    /*
     * If we get here, we know there is truly no possible palette solution since we exhausted every possibility. For
     * most reasonably sized tilesets, it would be difficult to reach this condition since there are too many possible
     * allocations to try. Instead it is more likely we hit the exploration cutoff case below.
     */
    if (printErrors) {
      fatalerror_noPossiblePaletteAssignment(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode);
    }
    return std::make_tuple(false, assignedPalsSolution, primaryPaletteColorSets);
  }
  else if (assignResult == AssignResult::EXPLORE_CUTOFF_REACHED) {
    if (printErrors) {
      fatalerror_assignExploreCutoffReached(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode, assignAlgorithm,
                                            exploredNodeCutoff);
    }
    return std::make_tuple(false, assignedPalsSolution, primaryPaletteColorSets);
  }
  pt_logln(ctx, stderr, "{} assigned all NormalizedPalettes successfully after {} iterations",
           assignAlgorithmString(assignAlgorithm), ctx.compilerContext.exploredNodeCounter);
  return std::make_tuple(true, assignedPalsSolution, primaryPaletteColorSets);
}

struct AssignParams {
  AssignAlgorithm assignAlgorithm;
  std::size_t exploredNodeCutoff;
  std::size_t bestBranches;
  bool smartPrune;
};

// TODO : add smartPrune entries once that is implemented
static const std::array<AssignParams, 40> MATRIX{
    // DFS, 1 million iterations
    AssignParams{AssignAlgorithm::DFS, 1'000'000, 2, false}, AssignParams{AssignAlgorithm::DFS, 1'000'000, 3, false},
    AssignParams{AssignAlgorithm::DFS, 1'000'000, 4, false}, AssignParams{AssignAlgorithm::DFS, 1'000'000, 5, false},
    AssignParams{AssignAlgorithm::DFS, 1'000'000, 6, false},
    // BFS, 1 million iterations
    AssignParams{AssignAlgorithm::BFS, 1'000'000, 2, false}, AssignParams{AssignAlgorithm::BFS, 1'000'000, 3, false},
    AssignParams{AssignAlgorithm::BFS, 1'000'000, 4, false}, AssignParams{AssignAlgorithm::BFS, 1'000'000, 5, false},
    AssignParams{AssignAlgorithm::BFS, 1'000'000, 6, false},
    // DFS, 2 million iterations
    AssignParams{AssignAlgorithm::DFS, 2'000'000, 2, false}, AssignParams{AssignAlgorithm::DFS, 2'000'000, 3, false},
    AssignParams{AssignAlgorithm::DFS, 2'000'000, 4, false}, AssignParams{AssignAlgorithm::DFS, 2'000'000, 5, false},
    AssignParams{AssignAlgorithm::DFS, 2'000'000, 6, false},
    // BFS, 2 million iterations
    AssignParams{AssignAlgorithm::BFS, 2'000'000, 2, false}, AssignParams{AssignAlgorithm::BFS, 2'000'000, 3, false},
    AssignParams{AssignAlgorithm::BFS, 2'000'000, 4, false}, AssignParams{AssignAlgorithm::BFS, 2'000'000, 5, false},
    AssignParams{AssignAlgorithm::BFS, 2'000'000, 6, false},
    // DFS, 4 million iterations
    AssignParams{AssignAlgorithm::DFS, 4'000'000, 2, false}, AssignParams{AssignAlgorithm::DFS, 4'000'000, 3, false},
    AssignParams{AssignAlgorithm::DFS, 4'000'000, 4, false}, AssignParams{AssignAlgorithm::DFS, 4'000'000, 5, false},
    AssignParams{AssignAlgorithm::DFS, 4'000'000, 6, false},
    // BFS, 4 million iterations
    AssignParams{AssignAlgorithm::BFS, 4'000'000, 2, false}, AssignParams{AssignAlgorithm::BFS, 4'000'000, 3, false},
    AssignParams{AssignAlgorithm::BFS, 4'000'000, 4, false}, AssignParams{AssignAlgorithm::BFS, 4'000'000, 5, false},
    AssignParams{AssignAlgorithm::BFS, 4'000'000, 6, false},
    // DFS, 8 million iterations
    AssignParams{AssignAlgorithm::DFS, 8'000'000, 2, false}, AssignParams{AssignAlgorithm::DFS, 8'000'000, 3, false},
    AssignParams{AssignAlgorithm::DFS, 8'000'000, 4, false}, AssignParams{AssignAlgorithm::DFS, 8'000'000, 5, false},
    AssignParams{AssignAlgorithm::DFS, 8'000'000, 6, false},
    // BFS, 8 million iterations
    AssignParams{AssignAlgorithm::BFS, 8'000'000, 2, false}, AssignParams{AssignAlgorithm::BFS, 8'000'000, 3, false},
    AssignParams{AssignAlgorithm::BFS, 8'000'000, 4, false}, AssignParams{AssignAlgorithm::BFS, 8'000'000, 5, false},
    AssignParams{AssignAlgorithm::BFS, 8'000'000, 6, false}};

std::pair<std::vector<ColorSet>, std::vector<ColorSet>>
runPaletteAssignmentMatrix(PtContext &ctx, const std::vector<ColorSet> &colorSets,
                           const std::vector<ColorSet> &primerColorSets,
                           const std::unordered_map<BGR15, std::size_t> &colorToIndex)
{
  /*
   * First, we detect if we are in a command line override case. There are three of these.
   */

  /*
   * User is running compile-primary, we are compiling the primary, and user supplied an explicit override value. In
   * this case, we don't want to read anything from the assign config. Just return.
   */
  bool primaryOverride = ctx.subcommand == Subcommand::COMPILE_PRIMARY &&
                         ctx.compilerConfig.mode == CompilerMode::PRIMARY &&
                         ctx.compilerConfig.providedAssignConfigOverride;
  /*
   * User is running compile-secondary, we are compiling the secondary, and user supplied an explicit override value.
   * In this case, we don't want to read anything from the assign config. Just return.
   */
  bool secondaryOverride = ctx.subcommand == Subcommand::COMPILE_SECONDARY &&
                           ctx.compilerConfig.mode == CompilerMode::SECONDARY &&
                           ctx.compilerConfig.providedAssignConfigOverride;
  /*
   * User is running compile-secondary, we are compiling the paired primary, and user supplied an explicit primary
   * override value. In this case, we don't want to read anything from the assign config. Just return.
   */
  bool pairedPrimaryOverride = ctx.subcommand == Subcommand::COMPILE_SECONDARY &&
                               ctx.compilerConfig.mode == CompilerMode::PRIMARY &&
                               ctx.compilerConfig.providedPrimaryAssignConfigOverride;

  // If user supplied any command line overrides, we don't want to run the full matrix. Instead, die upon failure.
  if (primaryOverride || secondaryOverride || pairedPrimaryOverride) {
    auto [success, assignedPalsSolution, primaryPaletteColorSets] =
        tryAssignment(ctx, colorSets, primerColorSets, colorToIndex, true);
    if (success) {
      return std::pair{assignedPalsSolution, primaryPaletteColorSets};
    }
  }

  if ((ctx.compilerConfig.mode == CompilerMode::PRIMARY && ctx.compilerConfig.readCachedPrimaryConfig) ||
      (ctx.compilerConfig.mode == CompilerMode::SECONDARY && ctx.compilerConfig.readCachedSecondaryConfig)) {
    if (!ctx.compilerConfig.forceParamSearchMatrix) {
      /*
       * If we read a cached assignment setting that corresponds to our current compilation mode, try it first to
       * potentially save a ton of time.
       */
      auto assignmentResult = tryAssignment(ctx, colorSets, primerColorSets, colorToIndex, false);
      bool success = std::get<0>(assignmentResult);
      if (success) {
        auto assignedPalsSolution = std::get<1>(assignmentResult);
        auto primaryPaletteColorSets = std::get<2>(assignmentResult);
        return std::pair{assignedPalsSolution, primaryPaletteColorSets};
      }
      if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
        warn_invalidAssignConfigCache(ctx.err, ctx.compilerConfig, ctx.compilerSrcPaths.primaryAssignConfig());
      }
      else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
        warn_invalidAssignConfigCache(ctx.err, ctx.compilerConfig, ctx.compilerSrcPaths.secondaryAssignConfig());
      }
    }
  }
  else {
    if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
      warn_missingAssignConfig(ctx.err, ctx.compilerConfig, ctx.compilerSrcPaths.primaryAssignConfig());
    }
    else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
      warn_missingAssignConfig(ctx.err, ctx.compilerConfig, ctx.compilerSrcPaths.secondaryAssignConfig());
    }
  }

  for (std::size_t index = 0; index < MATRIX.size(); index++) {
    if (ctx.compilerConfig.mode == CompilerMode::PRIMARY) {
      ctx.compilerConfig.primaryAssignAlgorithm = MATRIX.at(index).assignAlgorithm;
      ctx.compilerConfig.primaryExploredNodeCutoff = MATRIX.at(index).exploredNodeCutoff;
      ctx.compilerConfig.primaryBestBranches = MATRIX.at(index).bestBranches;
      ctx.compilerConfig.primarySmartPrune = MATRIX.at(index).smartPrune;
    }
    else if (ctx.compilerConfig.mode == CompilerMode::SECONDARY) {
      ctx.compilerConfig.secondaryAssignAlgorithm = MATRIX.at(index).assignAlgorithm;
      ctx.compilerConfig.secondaryExploredNodeCutoff = MATRIX.at(index).exploredNodeCutoff;
      ctx.compilerConfig.secondaryBestBranches = MATRIX.at(index).bestBranches;
      ctx.compilerConfig.secondarySmartPrune = MATRIX.at(index).smartPrune;
    }
    auto [success, assignedPalsSolution, primaryPaletteColorSets] =
        tryAssignment(ctx, colorSets, primerColorSets, colorToIndex, false);
    if (success) {
      return std::pair{assignedPalsSolution, primaryPaletteColorSets};
    }
  }
  // If we got here, the matrix failed, print a sad message
  fatalerror_paletteAssignParamSearchMatrixFailed(ctx.err, ctx.compilerSrcPaths, ctx.compilerConfig.mode);
  // unreachable, here for compiler
  throw std::runtime_error("assign param matrix failed :-(");
}

} // namespace porytiles
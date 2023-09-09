#include "palette_assignment.h"

#include <deque>
#include <unordered_set>
#include <vector>

#include "logger.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

AssignResult assignDepthFirst(PtContext &ctx, AssignState &state, std::vector<ColorSet> &solution,
                              const std::vector<ColorSet> &primaryPalettes)
{
  ctx.compilerContext.exploredNodeCounter++;
  if (ctx.compilerContext.exploredNodeCounter % EXPLORATION_CUTOFF_MULTIPLIER == 0) {
    pt_logln(ctx, stderr, "exploredNodeCounter passed factor {}",
             ctx.compilerContext.exploredNodeCounter / EXPLORATION_CUTOFF_MULTIPLIER);
  }
  if (ctx.compilerContext.exploredNodeCounter > ctx.compilerConfig.exploredNodeCutoff) {
    return AssignResult::EXPLORE_CUTOFF_REACHED;
  }

  if (state.unassigned.empty()) {
    // No tiles left to assign, found a solution!
    std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes), std::back_inserter(solution));
    return AssignResult::SUCCESS;
  }

  /*
   * We will try to assign the last element to one of the 6 hw palettes, last because it is a vector so easier to
   * add/remove from the end.
   */
  ColorSet &toAssign = state.unassigned.back();

  /*
   * If we are assigning a secondary set, we'll want to first check if any of the primary palettes satisfy the color
   * constraints for this particular tile. That way we can just use the primary palette, since those are available for
   * secondary tiles to freely use.
   */
  if (!primaryPalettes.empty()) {
    for (size_t i = 0; i < primaryPalettes.size(); i++) {
      const ColorSet &palette = primaryPalettes.at(i);
      if ((palette | toAssign).count() == palette.count()) {
        /*
         * This case triggers if `toAssign' shares all its colors with one of the palettes from the primary
         * tileset. In that case, we will just reuse that palette when we make the tile in a later step. So we
         * can prep a recursive call to assign with an unchanged state (other than removing `toAssign')
         */
        std::vector<ColorSet> unassignedCopy;
        std::copy(std::begin(state.unassigned), std::end(state.unassigned), std::back_inserter(unassignedCopy));
        std::vector<ColorSet> hardwarePalettesCopy;
        std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
                  std::back_inserter(hardwarePalettesCopy));
        unassignedCopy.pop_back();
        AssignState updatedState = {hardwarePalettesCopy, unassignedCopy};

        AssignResult result = assignDepthFirst(ctx, updatedState, solution, primaryPalettes);
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

  std::size_t stopLimit = state.hardwarePalettes.size();
  if (ctx.compilerConfig.smartPrune) {
    throw std::runtime_error{"TODO : impl smart prune"};
  }
  else if (ctx.compilerConfig.pruneCount > 0) {
    if (ctx.compilerConfig.pruneCount >= stopLimit) {
      // FIXME : proper error message
      throw std::runtime_error{"pruneCount pruned every branch"};
    }
    stopLimit -= ctx.compilerConfig.pruneCount;
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
    std::vector<ColorSet> unassignedCopy;
    std::copy(std::begin(state.unassigned), std::end(state.unassigned), std::back_inserter(unassignedCopy));
    std::vector<ColorSet> hardwarePalettesCopy;
    std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes),
              std::back_inserter(hardwarePalettesCopy));
    unassignedCopy.pop_back();
    hardwarePalettesCopy.at(i) |= toAssign;
    AssignState updatedState = {hardwarePalettesCopy, unassignedCopy};

    AssignResult result = assignDepthFirst(ctx, updatedState, solution, primaryPalettes);
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

AssignResult assignDepthFirstIndexOnly(PtContext &ctx, AssignStateIndexOnly &state, std::vector<ColorSet> &solution,
                                       const std::vector<ColorSet> &primaryPalettes,
                                       const std::vector<ColorSet> &unassigneds)
{
  ctx.compilerContext.exploredNodeCounter++;
  if (ctx.compilerContext.exploredNodeCounter % EXPLORATION_CUTOFF_MULTIPLIER == 0) {
    pt_logln(ctx, stderr, "exploredNodeCounter passed factor {}",
             ctx.compilerContext.exploredNodeCounter / EXPLORATION_CUTOFF_MULTIPLIER);
  }
  if (ctx.compilerContext.exploredNodeCounter > ctx.compilerConfig.exploredNodeCutoff) {
    return AssignResult::EXPLORE_CUTOFF_REACHED;
  }

  if (state.unassignedCount == 0) {
    // No tiles left to assign, found a solution!
    std::copy(std::begin(state.hardwarePalettes), std::end(state.hardwarePalettes), std::back_inserter(solution));
    return AssignResult::SUCCESS;
  }

  /*
   * We will try to assign the last element to one of the 6 hw palettes, last because it is a vector so easier to
   * add/remove from the end.
   */
  const ColorSet &toAssign = unassigneds.at(state.unassignedCount - 1);

  /*
   * If we are assigning a secondary set, we'll want to first check if any of the primary palettes satisfy the color
   * constraints for this particular tile. That way we can just use the primary palette, since those are available for
   * secondary tiles to freely use.
   */
  if (!primaryPalettes.empty()) {
    for (size_t i = 0; i < primaryPalettes.size(); i++) {
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
        hardwarePalettesCopy.at(i) |= toAssign;
        AssignStateIndexOnly updatedState = {hardwarePalettesCopy, state.unassignedCount - 1};

        AssignResult result = assignDepthFirstIndexOnly(ctx, updatedState, solution, primaryPalettes, unassigneds);
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

  std::size_t stopLimit = state.hardwarePalettes.size();
  if (ctx.compilerConfig.smartPrune) {
    throw std::runtime_error{"TODO : impl smart prune"};
  }
  else if (ctx.compilerConfig.pruneCount > 0) {
    if (ctx.compilerConfig.pruneCount >= stopLimit) {
      // FIXME : proper error message
      throw std::runtime_error{"pruneCount pruned every branch"};
    }
    stopLimit -= ctx.compilerConfig.pruneCount;
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
    AssignStateIndexOnly updatedState = {hardwarePalettesCopy, state.unassignedCount - 1};

    AssignResult result = assignDepthFirstIndexOnly(ctx, updatedState, solution, primaryPalettes, unassigneds);
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

AssignResult assignBreadthFirstIndexOnly(PtContext &ctx, AssignStateIndexOnly &initialState,
                                         std::vector<ColorSet> &solution, const std::vector<ColorSet> &primaryPalettes,
                                         const std::vector<ColorSet> &unassigneds)
{
  std::unordered_set<AssignStateIndexOnly> visitedStates{};
  std::deque<AssignStateIndexOnly> stateQueue{};
  std::deque<AssignStateIndexOnly> lowPriorityQueue{};
  stateQueue.push_back(initialState);
  visitedStates.insert(initialState);

  while (!stateQueue.empty() || !lowPriorityQueue.empty()) {
    AssignStateIndexOnly currentState;
    ctx.compilerContext.exploredNodeCounter++;
    if (ctx.compilerContext.exploredNodeCounter % EXPLORATION_CUTOFF_MULTIPLIER == 0) {
      pt_logln(ctx, stderr, "exploredNodeCounter passed factor {}, stateQueue={}, lowPrioQueue={}",
               ctx.compilerContext.exploredNodeCounter / EXPLORATION_CUTOFF_MULTIPLIER, stateQueue.size(),
               lowPriorityQueue.size());
    }
    if (ctx.compilerContext.exploredNodeCounter > ctx.compilerConfig.exploredNodeCutoff) {
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

    // FIXME : handle secondary sets properly, see how depth-first does it
    if (!primaryPalettes.empty()) {
      throw std::runtime_error{"TODO : support secondary set compilation with BFS backend"};
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
    std::size_t stopLimit = currentState.hardwarePalettes.size();
    if (ctx.compilerConfig.smartPrune) {
      // TODO : impl smart prune feature
      throw std::runtime_error{"TODO : impl smart prune"};
    }
    else if (ctx.compilerConfig.pruneCount > 0) {
      if (ctx.compilerConfig.pruneCount >= stopLimit) {
        // FIXME : proper error message
        throw std::runtime_error{"pruneCount pruned every branch"};
      }
      stopLimit -= ctx.compilerConfig.pruneCount;
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
      AssignStateIndexOnly updatedState = {hardwarePalettesCopy, currentState.unassignedCount - 1};
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

} // namespace porytiles
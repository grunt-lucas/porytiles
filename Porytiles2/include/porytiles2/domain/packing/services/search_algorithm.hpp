#pragma once

#include <format>
#include <ostream>
#include <string>

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Search algorithm used by BacktrackingStrategy.
 *
 * @details
 * Controls whether the backtracking search explores the solution space using depth-first or breadth-first traversal.
 * DFS uses in-place mutation with undo for memory efficiency, while BFS uses visited-state deduplication and a
 * dual-queue heuristic.
 */
enum class SearchAlgorithm {
    /**
     * @brief Depth-first search with in-place mutation and undo.
     */
    dfs,
    /**
     * @brief Breadth-first search with dual-queue heuristic and visited-state deduplication.
     */
    bfs
};

/**
 * @brief Converts a SearchAlgorithm to its canonical string representation.
 *
 * @param s The value to convert
 * @return The canonical snake_case string representation
 */
[[nodiscard]] inline std::string to_string(const SearchAlgorithm s)
{
    switch (s) {
    case SearchAlgorithm::dfs:
        return "dfs";
    case SearchAlgorithm::bfs:
        return "bfs";
    }
    panic("unhandled SearchAlgorithm value");
}

/**
 * @brief Stream insertion operator for SearchAlgorithm.
 *
 * @param os The output stream
 * @param s The value to output
 * @return Reference to the output stream
 */
inline std::ostream &operator<<(std::ostream &os, const SearchAlgorithm s)
{
    return os << to_string(s);
}

} // namespace porytiles2

template <>
struct std::formatter<porytiles2::SearchAlgorithm> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::SearchAlgorithm &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};

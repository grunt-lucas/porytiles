#pragma once

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

namespace porytiles2 {

/**
 * @brief Converts a map of counts to a sorted vector of key-count pairs.
 *
 * @details
 * This function transforms a map where keys are associated with counts into a vector of pairs, sorted in descending
 * order by count value. Items with higher counts appear first in the resulting vector. This is useful for prioritizing
 * or ranking items based on their frequency or occurrence count.
 *
 * @tparam T The type of the keys in the count map
 * @param counts A map where keys are associated with their count values
 * @return A vector of key-count pairs sorted in descending order by count
 * @post The returned vector contains all entries from the input map
 * @post The returned vector is sorted such that result[i].second >= result[i+1].second for all valid i
 */
template <typename T>
std::vector<std::pair<T, unsigned int>> counts_to_descending_list(const std::map<T, unsigned int> &counts)
{
    std::vector<std::pair<T, unsigned int>> result;
    result.reserve(counts.size());
    for (const auto &[key, count] : counts) {
        result.emplace_back(key, count);
    }

    // Sort in descending order by count (second element)
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return a.second > b.second; });

    return result;
}

} // namespace porytiles2

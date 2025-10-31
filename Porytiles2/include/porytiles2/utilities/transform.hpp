#pragma once

#include <ranges>
#include <type_traits>
#include <vector>

namespace porytiles2 {

/**
 * @brief Transforms a vector of type T into a vector of type U using a mapping function.
 *
 * @details
 * This function applies a transformation function to each element of the input vector, producing a new vector
 * containing the transformed elements. The transformation is performed using C++23 ranges for optimal performance. The
 * function uses lazy evaluation via std::views::transform and efficiently materializes the result with std::ranges::to.
 *
 * @tparam T The type of elements in the input vector
 * @tparam F The type of the transformation function
 * @param input The input vector to transform
 * @param func The transformation function that maps T to U
 * @return A new vector containing the transformed elements
 */
template <typename T, typename F>
[[nodiscard]] auto map(const std::vector<T> &input, F &&func) -> std::vector<std::invoke_result_t<F, const T &>>
{
    return input | std::views::transform(std::forward<F>(func)) | std::ranges::to<std::vector>();
}

} // namespace porytiles2

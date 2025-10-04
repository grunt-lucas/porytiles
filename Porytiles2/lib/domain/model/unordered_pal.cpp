#include "porytiles2/domain/model/unordered_pal.hpp"

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

void UnorderedPal::insert(const Rgba32 &color)
{
    if (!unique_.contains(color)) {
        colors_.push_back(color);
    }
}

[[nodiscard]] std::size_t UnorderedPal::size() const
{
    return colors_.size();
}

[[nodiscard]] bool UnorderedPal::has_identical_content(const UnorderedPal &other) const
{
    return unique_ == other.unique_;
}

} // namespace porytiles2

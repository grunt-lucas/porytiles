#pragma once

// ReSharper disable once CppUnusedIncludeDirective
#include <compare>
#include <set>

#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

class IndexPixel {
  public:
    IndexPixel() : index_{0} {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    IndexPixel(unsigned int index) : index_{index}
    {
        // TODO: don't hardcode 16 here
        // if (index_ >= 16) {
        //     panic("invalid IndexPixel value: " + std::to_string(index_));
        // }
    }

    bool operator==(const IndexPixel &other) const = default;

    auto operator<=>(const IndexPixel &) const = default;

    /**
     * @brief Checks if this indexed pixel is transparent.
     *
     * @details
     * In indexed color mode, palette index 0 is conventionally the transparent color.
     *
     * @return True if the palette index is 0, false otherwise
     */
    [[nodiscard]] bool is_transparent() const
    {
        return index_ == 0;
    }

    [[nodiscard]] unsigned int index() const
    {
        return index_;
    }

  private:
    unsigned int index_;
};

} // namespace porytiles2
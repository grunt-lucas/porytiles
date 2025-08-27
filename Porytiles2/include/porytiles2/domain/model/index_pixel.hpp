#pragma once

namespace porytiles2 {

class IndexPixel {
  public:
    IndexPixel() : index_{0} {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    IndexPixel(unsigned int index) : index_{index} {}

    [[nodiscard]] unsigned int index() const
    {
        return index_;
    }

    /**
     * @brief Checks if this indexed pixel is transparent.
     *
     * @details
     * In indexed color mode, palette index 0 is conventionally the transparent color.
     *
     * @param unused The extrinsic transparency value (unused for indexed pixels)
     * @return True if the palette index is 0, false otherwise
     */
    [[nodiscard]] bool is_transparent(const IndexPixel &unused) const
    {
        return index_ == 0;
    }

  private:
    unsigned int index_;
};

} // namespace porytiles2
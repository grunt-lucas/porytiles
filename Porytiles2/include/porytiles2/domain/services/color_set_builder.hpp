#pragma once

#include <map>

#include "gsl/pointers"

#include "porytiles2/domain/model/color_set.hpp"
#include "porytiles2/domain/model/normalized_pal.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Service that builds a ColorSet from a NormalizedPal using a pre-defined color index map.
 */
class ColorSetBuilder {
  public:
    /**
     * @brief Constructs a ColorSetBuilder with the specified text formatter.
     *
     * @details
     * The text formatter is used for generating diagnostic messages during the build process.
     *
     * @param format A non-null pointer to the TextFormatter for formatting diagnostic output
     */
    explicit ColorSetBuilder(gsl::not_null<TextFormatter *> format) : format_{format} {}

    /**
     * @brief Builds a ColorSet from a NormalizedPal using the provided color index map.
     *
     * @details
     * This method constructs a ColorSet by mapping each color in the NormalizedPal to its corresponding index value
     * from the color_index_map. The resulting ColorSet contains the indexed representation of the palette colors.
     *
     * @param pal The normalized palette containing RGBA32 colors to be indexed
     * @param color_index_map A map from RGBA32 colors to their corresponding index values
     * @return A ColorSet containing the indexed color data
     */
    [[nodiscard]] ColorSet
    build(const NormalizedPal<Rgba32> &pal, const std::map<Rgba32, unsigned int> &color_index_map) const;

  private:
    /**
     * @brief Text formatter for generating diagnostic messages.
     */
    TextFormatter *format_;
};

} // namespace porytiles2

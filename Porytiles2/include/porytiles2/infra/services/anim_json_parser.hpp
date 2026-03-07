#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/models/anim_params.hpp"
#include "porytiles2/utilities/dynamic_cased_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Parses and writes animation configuration JSON files (anim.json).
 *
 * @details
 * AnimJsonParser handles reading and writing of anim.json files, which define animation parameters for a tileset's
 * Porytiles component. The JSON structure maps animation names to their configuration parameters.
 *
 * Example anim.json structure:
 * ```json
 * {
 *   "flower": {
 *     "frame_factor": 16,
 *     "frame_offset": 1,
 *     "frames": ["0", "1", "0", "2"]
 *   },
 *   "water": {
 *     "frame_factor": 16,
 *     "frame_offset": 2,
 *     "frames": ["0", "1", "2", "3"]
 *   }
 * }
 * ```
 *
 * Fields:
 * - frame_factor: Modulus divisor for timer (default: 16)
 * - frame_offset: Remainder for timer modulo check (default: 0)
 * - frames: Array of frame names defining unique frames (default: ["0"])
 * - frame_order: Array of frame names defining playback order (default: same as frames)
 * - counter_max: Timer wrap-around value (default: 256, optional)
 *
 * Note: tile_offset and tile_count are NOT stored in anim.json - they are computed during compilation and stored in
 * generated_anim_code.h.
 */
class AnimJsonParser {
  public:
    explicit AnimJsonParser(gsl::not_null<const TextFormatter *> format);

    /**
     * @brief Parses an anim.json file into a map of animation parameters.
     *
     * @details
     * Reads the JSON file at the specified path and extracts animation parameters for each animation defined. Missing
     * optional fields use default values from AnimParams.
     *
     * @param json_path Path to the anim.json file
     * @return Map of animation name to AnimParams, or error if parsing fails
     */
    [[nodiscard]] ChainableResult<std::map<DynamicCasedName, AnimParams>>
    parse(const std::filesystem::path &json_path) const;

    /**
     * @brief Writes animation parameters to an anim.json file.
     *
     * @details
     * Serializes the provided animation parameters map to JSON format and writes to the specified path. Overwrites any
     * existing file at that location.
     *
     * @param json_path Path to write the anim.json file
     * @param params Map of animation name to AnimParams to serialize
     * @return Success or error if writing fails
     */
    [[nodiscard]] ChainableResult<void>
    write(const std::filesystem::path &json_path, const std::map<DynamicCasedName, AnimParams> &params) const;

  private:
    const TextFormatter *format_;
};

} // namespace porytiles2

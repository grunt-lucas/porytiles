#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Parses and writes animation configuration YAML files (anim.yaml).
 *
 * @details
 * AnimYamlParser handles reading and writing of anim.yaml files, which define animation parameters for a tileset's
 * Porytiles component. The YAML structure maps animation names to their configuration parameters.
 *
 * Example anim.yaml structure:
 * ```yaml
 * flower:
 *   frame_factor: 16
 *   frame_offset: 1
 *   frames: [0, 1, 0, 2]
 * water:
 *   frame_factor: 16
 *   frame_offset: 2
 *   frames: [0, 1, 2, 3]
 * ```
 *
 * Fields:
 * - frame_factor: Modulus divisor for timer (default: 16)
 * - frame_offset: Remainder for timer modulo check (default: 0)
 * - frames: Array of frame indices defining playback order (default: [0])
 * - counter_max: Timer wrap-around value (default: 256, optional)
 *
 * Note: tile_offset and tile_count are NOT stored in anim.yaml - they are computed during compilation and stored in
 * generated_anim_code.h.
 */
class AnimYamlParser {
  public:
    explicit AnimYamlParser(gsl::not_null<const TextFormatter *> format);

    /**
     * @brief Parses an anim.yaml file into a map of animation parameters.
     *
     * @details
     * Reads the YAML file at the specified path and extracts animation parameters for each animation defined. Missing
     * optional fields use default values from AnimationParams.
     *
     * @param yaml_path Path to the anim.yaml file
     * @return Map of animation name to AnimationParams, or error if parsing fails
     */
    [[nodiscard]] ChainableResult<std::map<std::string, AnimationParams>>
    parse(const std::filesystem::path &yaml_path) const;

    /**
     * @brief Writes animation parameters to an anim.yaml file.
     *
     * @details
     * Serializes the provided animation parameters map to YAML format and writes to the specified path. Overwrites any
     * existing file at that location.
     *
     * @param yaml_path Path to write the anim.yaml file
     * @param params Map of animation name to AnimationParams to serialize
     * @return Success or error if writing fails
     */
    [[nodiscard]] ChainableResult<void>
    write(const std::filesystem::path &yaml_path, const std::map<std::string, AnimationParams> &params) const;

  private:
    const TextFormatter *format_;
};

} // namespace porytiles2
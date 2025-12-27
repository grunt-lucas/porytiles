#pragma once

#include <any>

#include "porytiles2/domain/models/animation_callback_info.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for reading tileset artifacts from a backing store into a Tileset object.
 *
 * @details
 * The TilesetArtifactReader provides the capability to read various types of tileset artifacts (PNG files, binary data,
 * CSV files, etc.) from their stored locations and populate the appropriate fields in a Tileset object. This interface
 * abstracts the reading logic from the specific storage format and location.
 *
 * Implementations handle the details of parsing different artifact types and updating the correct components within the
 * target Tileset. The interface uses ArtifactKey to support different backing store implementations (filesystem paths,
 * database keys, URLs, etc.), as long as the key is string-representable.
 */
class TilesetArtifactReader {
  public:
    virtual ~TilesetArtifactReader() = default;

    /*
     * Porymap artifacts
     */
    [[nodiscard]] virtual ChainableResult<void> read_metatiles_bin(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_metatile_attributes_bin(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_tiles_png(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_porymap_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_porymap_anim_frame(
        Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index) const = 0;

    /**
     * @brief Reads animation parameters from C source code into the Porymap component.
     *
     * @details
     * Parses animation C code (either generated_anim_code.h or tileset_anims.c) to extract animation
     * parameters (tile offsets, tile counts, frame sequences, timing, etc.) and updates the AnimationParams
     * for each animation in the Porymap component. This only sets parameters; animation frame PNGs are
     * loaded separately.
     *
     * The callback_info parameter provides the actual callback function name from the tileset metadata,
     * ensuring we parse the correct animations rather than guessing based on naming conventions.
     *
     * @param dest The Tileset object to populate with animation parameters
     * @param callback_info Information about the animation callback from tileset metadata
     * @return Empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] virtual ChainableResult<void>
    read_anim_code(Tileset &dest, const AnimationCallbackInfo &callback_info) const = 0;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] virtual ChainableResult<void> read_bottom_png(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_middle_png(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_top_png(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_attributes_csv(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_porytiles_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_porytiles_anim_frame(
        Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, std::size_t frame_index) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_porytiles_anim_key_frame(Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name) const = 0;

    /**
     * @brief Reads animation parameters from an anim.yaml file into the Porytiles component.
     *
     * @details
     * Parses the anim.yaml file to extract animation parameters (frame sequences, timing, etc.) and updates the
     * AnimationParams for each animation in the Porytiles component. This only sets parameters; animation frame PNGs
     * are loaded separately.
     *
     * @param dest The Tileset object to populate with animation parameters
     * @param src_key Key identifying the anim.yaml artifact to read
     * @return Empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] virtual ChainableResult<void> read_anim_yaml(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_config(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_local_config(Tileset &dest, const ArtifactKey &src_key) const = 0;
};

} // namespace porytiles2

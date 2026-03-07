#pragma once

#include <any>
#include <string>
#include <utility>
#include <vector>

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

    /**
     * @brief Reads a complete Porymap animation (params + frames) into the Porymap component.
     *
     * @details
     * Parses animation parameters from C code (generated_anim_code.h or tileset_anims.c) for the
     * specified animation, loads all frame images, and constructs a complete Animation in the
     * Porymap component.
     *
     * @param dest The Tileset object to populate
     * @param anim_name The name of the animation to load
     * @param params_key Key to the C code file containing animation parameters
     * @param frame_keys Ordered list of (frame_name, artifact_key) pairs for each unique frame
     * @return Empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] virtual ChainableResult<void> read_porymap_anim(
        Tileset &dest,
        const std::string &anim_name,
        const ArtifactKey &params_key,
        const std::vector<std::pair<std::string, ArtifactKey>> &frame_keys) const = 0;

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

    /**
     * @brief Reads a complete Porytiles animation (params + frames) into the Porytiles component.
     *
     * @details
     * Parses animation parameters from anim.json for the specified animation, loads all frame images,
     * and constructs a complete Animation in the Porytiles component.
     *
     * @param dest The Tileset object to populate
     * @param anim_name The name of the animation to load
     * @param params_key Key to the anim.json file
     * @param key_frame_key Key to the key frame
     * @param frame_keys Ordered list of (frame_name, artifact_key) pairs for each unique frame
     * @return Empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] virtual ChainableResult<void> read_porytiles_anim(
        Tileset &dest,
        const std::string &anim_name,
        const ArtifactKey &params_key,
        const ArtifactKey &key_frame_key,
        const std::vector<std::pair<std::string, ArtifactKey>> &frame_keys) const = 0;
};

} // namespace porytiles2

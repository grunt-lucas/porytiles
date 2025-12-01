#pragma once

#include <any>

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
    read_pal_n(Tileset &dest, const ArtifactKey &src_key, unsigned int index) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_porymap_anim_frame(
        Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, int frame_index) const = 0;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] virtual ChainableResult<void> read_bottom_png(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_middle_png(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_top_png(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_attributes_csv(Tileset &dest, const ArtifactKey &src_key) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_pal_override_n(Tileset &dest, const ArtifactKey &src_key, unsigned int index) const = 0;

    [[nodiscard]] virtual ChainableResult<void> read_porytiles_anim_frame(
        Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name, int frame_index) const = 0;
};

} // namespace porytiles2

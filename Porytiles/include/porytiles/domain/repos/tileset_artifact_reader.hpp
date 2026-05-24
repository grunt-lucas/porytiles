#pragma once

#include <any>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

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

    [[nodiscard]] virtual ChainableResult<void> read_porytiles_anim(
        Tileset &dest,
        const std::string &anim_name,
        const ArtifactKey &params_key,
        const std::optional<ArtifactKey> &key_frame_key,
        const std::vector<std::pair<std::string, ArtifactKey>> &frame_keys) const = 0;

    [[nodiscard]] virtual ChainableResult<void>
    read_porytiles_primary_anim_references(Tileset &dest, const ArtifactKey &params_key) const = 0;
};

} // namespace porytiles

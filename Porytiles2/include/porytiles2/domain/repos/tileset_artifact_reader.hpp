#pragma once

#include <any>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/templates/result.hpp"

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
 * target Tileset. The interface uses type-erased keys (std::any) to support different backing store implementations
 * (filesystem paths, database keys, URLs, etc.).
 */
class TilesetArtifactReader {
  public:
    virtual ~TilesetArtifactReader() = default;

    /**
     * @brief Reads an artifact from the backing store and updates the target Tileset.
     *
     * @details
     * This method reads the specified artifact from the backing store location identified by the src_key and updates
     * the appropriate fields or components within the destination Tileset object. The artifact parameter specifies the
     * type and metadata needed to determine how to read and where to store the data.
     *
     * The implementation should handle parsing the specific artifact format (PNG images, binary data, CSV files, etc.)
     * and updating the correct Tileset components (Porymap or Porytiles components, palettes, animations, etc.).
     *
     * @param dest The Tileset object to be updated with the read artifact data
     * @param src_key The key identifying the artifact location in the backing store
     * @param artifact The artifact specification including type and optional metadata
     * @return Empty Result on success, otherwise an error description
     */
    virtual Result<void> read(Tileset &dest, const std::any &src_key, const TilesetArtifact &artifact) = 0;
};

} // namespace porytiles2

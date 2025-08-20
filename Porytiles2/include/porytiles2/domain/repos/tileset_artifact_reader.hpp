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
 * target Tileset. The interface uses ArtifactKey to support different backing store implementations (filesystem paths,
 * database keys, URLs, etc.), as long as the key is string-representable.
 */
class TilesetArtifactReader {
  public:
    virtual ~TilesetArtifactReader() = default;

    /**
     * @brief Reads an artifact from the backing store and updates the target Tileset.
     *
     * @details
     * This method reads the specified artifact from the backing store location identified by the src_key and updates
     * the appropriate fields or components within the destination Tileset object. The TilesetArtifact parameter
     * specifies the type and metadata needed to determine how to read and where to store the data. The implementation
     * should handle parsing the specific artifact format (PNG images, binary data, CSV files, etc.) and updating the
     * correct Tileset components (Porymap or Porytiles components, palettes, animations, etc.).
     *
     * Precondition: the TilesetRepo checks that src_key actually exists before performing a read. Thus, the
     * TilesetArtifactReader's read method can assume the specified artifact really does exist. If the artifact does not
     * exist, the result is implementation-defined but will probably panic.
     *
     * @param dest The Tileset object to be updated with the read artifact data
     * @param src_key The ArtifactKey identifying the artifact location in the backing store
     * @param artifact The TilesetArtifact specification including type and optional metadata
     * @return Empty Result on success, otherwise an error description
     */
    virtual Result<void> read(Tileset &dest, const ArtifactKey &src_key, const TilesetArtifact &artifact) const = 0;
};

} // namespace porytiles2

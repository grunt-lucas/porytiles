#pragma once

#include <any>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for writing tileset artifacts from a Tileset object to a backing store.
 *
 * @details
 * The TilesetArtifactWriter provides the capability to extract data from a Tileset object and write various types of
 * tileset artifacts (PNG files, binary data, etc.) to their storage locations. This interface abstracts the writing
 * logic from the specific storage format and location.
 *
 * Implementations handle the details of formatting different artifact types and extracting the appropriate data from
 * the source Tileset components. The interface uses type-erased keys (std::any) to support different backing store
 * implementations (filesystem paths, database keys, URLs, etc.).
 */
class TilesetArtifactWriter {
  public:
    virtual ~TilesetArtifactWriter() = default;

    /**
     * @brief Writes an artifact from a Tileset to the backing store.
     *
     * @details
     * This method extracts the appropriate data from the source Tileset object and writes the specified artifact to the
     * backing store location identified by the dest_key. The artifact parameter specifies the type and metadata needed
     * to determine what data to extract and how to format it.
     *
     * The implementation should handle formatting the specific artifact type (PNG images, binary data, etc.) and
     * extracting the correct data from the Tileset components (Porymap or Porytiles components, palettes, animations,
     * etc.).
     *
     * @param dest_key The key identifying the destination location in the backing store
     * @param artifact The artifact specification including type and optional metadata
     * @param src The Tileset object containing the data to be written
     * @return Empty Result on success, otherwise an error description
     */
    virtual Result<void> write(const std::any &dest_key, const TilesetArtifact &artifact, const Tileset &src) = 0;
};

} // namespace porytiles2

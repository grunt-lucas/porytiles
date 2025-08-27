#pragma once

#include <any>

#include "porytiles2/domain/model/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for writing tileset artifacts from a Tileset object to a backing store.
 *
 * @details
 * The TilesetArtifactWriter provides the ability to extract data from a Tileset object and write various types of
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
     * @brief Begins a new transaction for atomic write operations.
     *
     * @details
     * Starts a transaction that buffers all subsequent write operations until a commit() call. If a transaction is
     * already active, this should return an error.
     *
     * @return Empty Result on success, otherwise an error description
     */
    [[nodiscard]] virtual Result<void> begin_transaction() = 0;

    /**
     * @brief Commits all buffered write operations in the current transaction.
     *
     * @details
     * Finalizes and persists all write operations that were buffered since begin_transaction() was called. After
     * commit, the transaction is closed and a new one must be started for further transactional writes. If no
     * transaction is active, this should return an error.
     *
     * @return Empty Result on success, otherwise an error description
     */
    [[nodiscard]] virtual Result<void> commit() = 0;

    /**
     * @brief Rolls back all buffered write operations in the current transaction.
     *
     * @details
     * Discards all write operations that were buffered since begin_transaction() was called. After rollback, the
     * transaction is closed and a new one must be started for further transactional writes. If no transaction is
     * active, this should return an error.
     *
     * @return Empty Result on success, otherwise an error description
     */
    [[nodiscard]] virtual Result<void> rollback() = 0;

    /**
     * @brief Writes an artifact from a Tileset to the backing store.
     *
     * @details
     * This method extracts the appropriate data from the source Tileset object and writes the specified artifact to the
     * backing store location identified by the dest_key. The TilesetArtifact parameter specifies the type and metadata
     * needed to determine what data to extract and how to format it.
     *
     * The implementation should handle formatting the specific artifact type (PNG images, binary data, etc.) and
     * extracting the correct data from the Tileset components (Porymap or Porytiles components, palettes, animations,
     * etc.).
     *
     * If a transaction is active (via begin_transaction()), the write should buffer until a commit() call. If no
     * transaction is active, the behavior is implementation-defined (immediate write or error).
     *
     * @param dest_key The ArtifactKey identifying the destination location in the backing store
     * @param artifact The TilesetArtifact specification including type and optional metadata
     * @param src The Tileset object containing the data to be written
     * @return Empty Result on success, otherwise an error description
     */
    [[nodiscard]] virtual Result<void>
    write(const ArtifactKey &dest_key, const TilesetArtifact &artifact, const Tileset &src) = 0;
};

} // namespace porytiles2

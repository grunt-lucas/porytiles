#pragma once

#include <any>

#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

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
     * @return Empty ChainableResult on success, otherwise an error description
     */
    [[nodiscard]] virtual ChainableResult<void> begin_transaction() = 0;

    /**
     * @brief Commits all buffered write operations in the current transaction.
     *
     * @details
     * Finalizes and persists all write operations that were buffered since begin_transaction() was called. After
     * commit, the transaction is closed and a new one must be started for further transactional writes. If no
     * transaction is active, this should return an error.
     *
     * @return Empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] virtual ChainableResult<void> commit() = 0;

    /**
     * @brief Rolls back all buffered write operations in the current transaction.
     *
     * @details
     * Discards all write operations that were buffered since begin_transaction() was called. After rollback, the
     * transaction is closed and a new one must be started for further transactional writes. If no transaction is
     * active, this should return an error.
     *
     * @return Empty ChainableResult on success, otherwise an error description
     */
    [[nodiscard]] virtual ChainableResult<void> rollback() = 0;

    /*
     * Porymap artifacts
     */
    [[nodiscard]] virtual ChainableResult<void>
    write_metatiles_bin(const ArtifactKey &dest_key, const Tileset &src) = 0;

    [[nodiscard]] virtual ChainableResult<void>
    write_metatile_attributes_bin(const ArtifactKey &dest_key, const Tileset &src) = 0;

    [[nodiscard]] virtual ChainableResult<void> write_tiles_png(const ArtifactKey &dest_key, const Tileset &src) = 0;

    [[nodiscard]] virtual ChainableResult<void>
    write_porymap_pal_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index) = 0;

    [[nodiscard]] virtual ChainableResult<void> write_porymap_anim_frame(
        const ArtifactKey &dest_key,
        const Tileset &src,
        const std::string &anim_name,
        const std::string &frame_name) = 0;

    /**
     * @brief Writes the animation parameters to the Porymap component backing store.
     *
     * @details
     * Generates and writes the animation parameters for all animations in the Porymap component.
     * The tileset_root_key provides the path used for constructing INCBIN statements.
     *
     * @param dest_key Key identifying where to write the parameters
     * @param src The Tileset object to extract animation data from
     * @return Empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] virtual ChainableResult<void>
    write_porymap_anim_params(const ArtifactKey &dest_key, const Tileset &src) = 0;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] virtual ChainableResult<void> write_bottom_png(const ArtifactKey &dest_key, const Tileset &src) = 0;

    [[nodiscard]] virtual ChainableResult<void> write_middle_png(const ArtifactKey &dest_key, const Tileset &src) = 0;

    [[nodiscard]] virtual ChainableResult<void> write_top_png(const ArtifactKey &dest_key, const Tileset &src) = 0;

    [[nodiscard]] virtual ChainableResult<void>
    write_attributes_csv(const ArtifactKey &dest_key, const Tileset &src) = 0;

    [[nodiscard]] virtual ChainableResult<void>
    write_porytiles_pal_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index) = 0;

    [[nodiscard]] virtual ChainableResult<void> write_porytiles_anim_frame(
        const ArtifactKey &dest_key,
        const Tileset &src,
        const std::string &anim_name,
        const std::string &frame_name) = 0;

    /**
     * @brief Writes animation parameters to the Porytiles component backing store.
     *
     * @details
     * Generates and writes the animation parameters (frame sequences, timing, etc.) for all animations in the Porytiles
     * component.
     *
     * @param dest_key Key identifying where to write the parameters
     * @param src The Tileset object from which to extract animation parameters
     * @return Empty ChainableResult on success, otherwise an error chain
     */
    [[nodiscard]] virtual ChainableResult<void>
    write_porytiles_anim_params(const ArtifactKey &dest_key, const Tileset &src) = 0;
};

} // namespace porytiles

#pragma once

#include <optional>
#include <set>
#include <string>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Abstract interface for generating keys and discovering tileset artifacts in a backing store.
 *
 * @details
 * This interface provides the key generation and discovery functionality needed by the tileset repository system.
 * Implementations handle the specifics of how to organize and access artifacts in different backing stores (filesystem,
 * database, etc.).
 *
 * The interface separates key generation (which should be fast and stateless) from discovery operations (which may
 * involve I/O to search the backing store). Implementations may choose to cache expensive lookups during construction
 * to optimize performance.
 *
 * From the interface's perspective, tileset artifact keys are simple strings wrapped by the ArtifactKey class. If a
 * particular implementation requires a complex key type, the implementation must take care to ensure that this key type
 * is string-constructable. Luckily, rich string-based formats like JSON should be able to support almost any imaginable
 * key schema.
 */
class TilesetArtifactKeyProvider {
  public:
    virtual ~TilesetArtifactKeyProvider() = default;

    /*
     * TODO: fix palette and anim frame reading. Pals have a 2 digit format, e.g.
     *
     * 00.pal, 09.pal, 12.pal
     *
     * Anim frames have a single digit format, e.g.
     *
     * 0.png, 9.png, 12.png
     */

    /*
     * Porymap artifacts
     */
    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_metatiles_bin(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey>
    key_for_metatile_attributes_bin(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_tiles_png(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey>
    key_for_porymap_pal_n(const std::string &tileset_name, std::size_t index) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_porymap_anim_frame(
        const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const = 0;

    /**
     * @brief Returns the key for Porymap animation parameters.
     *
     * @details
     * For Porymap animations, generated_anim_code.h is the source of truth for animation
     * parameters. For first-time imports where this file doesn't exist, the reader will
     * fall back to tileset_anims.c. This method is an alias for key_for_generated_anim_code()
     * but with a more explicit name for the animation loading context.
     *
     * @param tileset_name The name of the tileset
     * @return Key for the generated_anim_code.h file
     */
    [[nodiscard]] virtual ChainableResult<ArtifactKey>
    key_for_porymap_anim_params(const std::string &tileset_name) const = 0;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_bottom_png(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_middle_png(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_top_png(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey>
    key_for_attributes_csv(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey>
    key_for_porytiles_pal_n(const std::string &tileset_name, std::size_t index) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_porytiles_anim_frame(
        const std::string &tileset_name, const std::string &anim_name, const std::string &frame_name) const = 0;

    /**
     * @brief Returns the key for Porytiles animation parameters.
     *
     * @details
     * For Porytiles animations, the anim parameters store is the source of truth for animation names, frame sequences,
     * timing, and other parameters.
     *
     * @param tileset_name The name of the tileset
     * @return Key for the animation parameters store
     */
    [[nodiscard]] virtual ChainableResult<ArtifactKey>
    key_for_porytiles_anim_params(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_config(const std::string &tileset_name) const = 0;

    [[nodiscard]] virtual ChainableResult<ArtifactKey> key_for_local_config(const std::string &tileset_name) const = 0;

    /*
     * Utilities
     */

    /**
     * @brief Checks whether an artifact exists in the backing store for the given key.
     *
     * @details
     * This method should perform the actual existence check against the backing store using the provided key. This may
     * involve filesystem operations, database queries, or other I/O depending on the implementation.
     *
     * @param key The key identifying the artifact to check (generated by key_for)
     * @return True if the artifact exists in the backing store, false otherwise
     */
    [[nodiscard]] virtual bool artifact_exists(const ArtifactKey &key) const = 0;

    /**
     * @brief Discovers the names of all Porymap animations available for a tileset.
     *
     * @details
     * Searches the backing store to find all Porymap animation assets for the specified tileset. Porymap animations do
     * not require key frames, only sequential frame files.
     *
     * @param tileset_name The name of the tileset to search for animations
     * @return Set of animation names found in the backing store
     */
    [[nodiscard]] virtual ChainableResult<std::set<std::string>>
    discover_porymap_anims(const std::string &tileset_name) const = 0;

    /**
     * @brief Discovers the frame indices for a specific Porymap animation.
     *
     * @details
     * Searches the backing store to find all animation frame files (excluding the key frame and 00.png) for a specific
     * Porymap animation. The returned indices should be consecutive starting from 1. Since frame 0 is required, callers
     * don't need to discover it.
     *
     * @param tileset_name The name of the tileset containing the animation
     * @param anim_name The name of the animation to search for frames
     * @return Set of frames found in the backing store
     */
    [[nodiscard]] virtual ChainableResult<std::set<std::string>>
    discover_porymap_anim_frames(const std::string &tileset_name, const std::string &anim_name) const = 0;

    /**
     * @brief Discovers the names of all Porytiles animations available for a tileset.
     *
     * @details
     * Searches the backing store to find all Porytiles animation directories or assets for the specified tileset.
     * Porytiles animations require both a key frame and at least one animation frame (00.png).
     *
     * @param tileset_name The name of the tileset to search for animations
     * @return Set of animation names found in the backing store
     */
    [[nodiscard]] virtual ChainableResult<std::set<std::string>>
    discover_porytiles_anims(const std::string &tileset_name) const = 0;

    /**
     * @brief Discovers the frame indices for a specific Porytiles animation.
     *
     * @details
     * Searches the backing store to find all animation frame files (excluding the key frame and 00.png) for a specific
     * Porytiles animation. The returned indices should be consecutive starting from 1. Since frame 0 is required,
     * callers don't need to discover it.
     *
     * @param tileset_name The name of the tileset containing the animation
     * @param anim_name The name of the animation to search for frames
     * @return Set of frames found in the backing store (typically starting from 1)
     */
    [[nodiscard]] virtual ChainableResult<std::set<std::string>>
    discover_porytiles_anim_frames(const std::string &tileset_name, const std::string &anim_name) const = 0;

    /**
     * @brief Gets the keys for all Porymap artifacts present in the given Tileset.
     *
     * @details
     * Each Porymap artifact has a unique key by which the ArtifactChecksumProvider and the TilesetRepo can identify it.
     * The format of these keys and the method for producing them are implementation-defined. This method will only
     * return keys that actually exist in the backing store.
     *
     * @return A vector of Porymap artifact keys for the given Tileset
     */
    [[nodiscard]] virtual ChainableResult<std::vector<ArtifactKey>>
    get_porymap_artifact_keys(const std::string &tileset_name) const
    {
        std::vector<ArtifactKey> result;

        PT_TRY_ASSIGN_CHAIN_ERR(
            metatiles_key,
            key_for_metatiles_bin(tileset_name),
            "failed to get Porymap artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(metatiles_key)) {
            result.push_back(metatiles_key);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            attr_key,
            key_for_metatile_attributes_bin(tileset_name),
            "failed to get Porymap artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(attr_key)) {
            result.push_back(attr_key);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            tiles_png_key,
            key_for_tiles_png(tileset_name),
            "failed to get Porymap artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(tiles_png_key)) {
            result.push_back(tiles_png_key);
        }

        // TODO: warn user if we found pals like 1.pal, these won't work they have to be 01.pal
        for (std::size_t i = 0; i < pal::num_pals; i++) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                pal_key,
                key_for_porymap_pal_n(tileset_name, i),
                "failed to get Porymap artifact keys",
                std::vector<ArtifactKey>);
            if (artifact_exists(pal_key)) {
                result.push_back(pal_key);
            }
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            porymap_anims,
            discover_porymap_anims(tileset_name),
            "failed to get Porymap artifact keys",
            std::vector<ArtifactKey>);
        for (const auto &anim : porymap_anims) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frames,
                discover_porymap_anim_frames(tileset_name, anim),
                "failed to get Porymap artifact keys",
                std::vector<ArtifactKey>);
            for (const auto &frame : frames) {
                PT_TRY_ASSIGN_CHAIN_ERR(
                    frame_n_key,
                    key_for_porymap_anim_frame(tileset_name, anim, frame),
                    "failed to get Porymap artifact keys",
                    std::vector<ArtifactKey>);
                if (artifact_exists(frame_n_key)) {
                    result.push_back(frame_n_key);
                }
            }
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            generated_anim_code_key,
            key_for_porymap_anim_params(tileset_name),
            "failed to get Porymap artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(generated_anim_code_key)) {
            result.push_back(generated_anim_code_key);
        }

        return result;
    }

    /**
     * @brief Gets the keys for all Porytiles artifacts present in the given Tileset.
     *
     * @details
     * Each Porytiles artifact has a unique key by which the ArtifactChecksumProvider and the TilesetRepo can identify
     * it. The format of these keys and the method for producing them are implementation-defined. This method will only
     * return keys that actually exist in the backing store.
     *
     * @return A vector of Porytiles artifact keys for the given Tileset
     */
    [[nodiscard]] virtual ChainableResult<std::vector<ArtifactKey>>
    get_porytiles_artifact_keys(const std::string &tileset_name) const
    {
        /*
         * TODO: it feels like the discovery logic here and in tileset_repo.cpp is duplicated. Is there some way to
         * massage these two classes so we don't need to duplicate the logic in two places? Perhaps ArtifactKey should
         * also store a std::variant<TilesetArtifact, LayoutArtifact> as metadata. Then, tileset_repo.cpp could call
         * get_all_artifact_keys directly. And then search the output for the metadata it needs and throw if essential
         * items are missing?
         */

        std::vector<ArtifactKey> result;

        PT_TRY_ASSIGN_CHAIN_ERR(
            bottom_png_key,
            key_for_bottom_png(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(bottom_png_key)) {
            result.push_back(bottom_png_key);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            middle_png_key,
            key_for_middle_png(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(middle_png_key)) {
            result.push_back(middle_png_key);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            top_png_key,
            key_for_top_png(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(top_png_key)) {
            result.push_back(top_png_key);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            attr_csv_key,
            key_for_attributes_csv(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(attr_csv_key)) {
            result.push_back(attr_csv_key);
        }

        // TODO: warn user if we found Porytiles pals like 1.pal, these won't work they have to be 01.pal
        for (std::size_t i = 0; i < pal::num_pals; i++) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                override_key,
                key_for_porytiles_pal_n(tileset_name, i),
                "failed to get Porytiles artifact keys",
                std::vector<ArtifactKey>);
            if (artifact_exists(override_key)) {
                result.push_back(override_key);
            }
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_anims,
            discover_porytiles_anims(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        for (const auto &anim : porytiles_anims) {
            // TODO: don't hardcode key here
            PT_TRY_ASSIGN_CHAIN_ERR(
                key_frame_key,
                key_for_porytiles_anim_frame(tileset_name, anim, "key"),
                "tileset load failed",
                std::vector<ArtifactKey>);

            PT_TRY_ASSIGN_CHAIN_ERR(
                frames,
                discover_porytiles_anim_frames(tileset_name, anim),
                "failed to get Porytiles artifact keys",
                std::vector<ArtifactKey>);
            for (const auto &frame : frames) {
                PT_TRY_ASSIGN_CHAIN_ERR(
                    frame_n_key,
                    key_for_porytiles_anim_frame(tileset_name, anim, frame),
                    "failed to get Porytiles artifact keys",
                    std::vector<ArtifactKey>);
                if (artifact_exists(frame_n_key)) {
                    result.push_back(frame_n_key);
                }
            }
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            anim_yaml_key,
            key_for_porytiles_anim_params(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(anim_yaml_key)) {
            result.push_back(anim_yaml_key);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            config_key,
            key_for_config(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(config_key)) {
            result.push_back(config_key);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            local_config_key,
            key_for_local_config(tileset_name),
            "failed to get Porytiles artifact keys",
            std::vector<ArtifactKey>);
        if (artifact_exists(local_config_key)) {
            result.push_back(local_config_key);
        }

        return result;
    }

    /**
     * @brief Gets the keys for all tileset artifacts (both Porytiles and Porymap) present in the given Tileset.
     *
     * @details
     * This method combines the results from both get_porytiles_artifact_keys() and get_porymap_artifact_keys() to
     * provide a comprehensive list of all artifact keys associated with the tileset. This method will only
     * return keys that actually exist in the backing store.
     *
     * @param tileset_name The name of the Tileset for which to get all artifact keys
     * @return A vector containing all Porytiles and Porymap artifact keys for the given Tileset
     */
    [[nodiscard]] virtual ChainableResult<std::vector<ArtifactKey>>
    get_all_artifact_keys(const std::string &tileset_name) const
    {
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_keys,
            get_porytiles_artifact_keys(tileset_name),
            "failed to get all artifact keys",
            std::vector<ArtifactKey>);
        PT_TRY_ASSIGN_CHAIN_ERR(
            porymap_keys,
            get_porymap_artifact_keys(tileset_name),
            "failed to get all artifact keys",
            std::vector<ArtifactKey>);

        std::vector<ArtifactKey> result;
        result.reserve(porytiles_keys.size() + porymap_keys.size());
        result.insert(result.end(), porytiles_keys.begin(), porytiles_keys.end());
        result.insert(result.end(), porymap_keys.begin(), porymap_keys.end());

        return result;
    }
};

} // namespace porytiles2

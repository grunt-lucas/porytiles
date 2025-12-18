#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/repos/tileset_artifact_writer.hpp"
#include "porytiles2/infra/config/infra_config.hpp"
#include "porytiles2/infra/services/file_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Provides a filesystem-based implementation for TilesetArtifactWriter.
 *
 * @details
 * This class implements the TilesetArtifactWriter interface to provide writing functionality for tileset artifacts. It
 * operates within the context of a Pokémon Gen III decompilation project on the local filesystem.
 */
class ProjectTilesetArtifactWriter final : public TilesetArtifactWriter {
  public:
    ProjectTilesetArtifactWriter(
        gsl::not_null<InfraConfig *> config,
        std::filesystem::path project_root,
        gsl::not_null<PngRgbaImageSaver *> png_rgba_saver,
        gsl::not_null<PngIndexedImageSaver *> png_indexed_saver,
        gsl::not_null<FilePalSaver *> pal_saver)
        : config_{config}, project_root_{std::move(project_root)}, png_rgba_saver_{png_rgba_saver},
          png_indexed_saver_{png_indexed_saver}, pal_saver_{pal_saver}
    {
    }

    [[nodiscard]] ChainableResult<void> begin_transaction() override;

    [[nodiscard]] ChainableResult<void> commit() override;

    [[nodiscard]] ChainableResult<void> rollback() override;

    /*
     * Porymap artifacts
     */
    [[nodiscard]] ChainableResult<void> write_metatiles_bin(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void>
    write_metatile_attributes_bin(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_tiles_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void>
    write_porymap_pal_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index) override;

    [[nodiscard]] ChainableResult<void> write_porymap_anim_frame(
        const ArtifactKey &dest_key,
        const Tileset &src,
        const std::string &anim_name,
        std::size_t frame_index) override;

    [[nodiscard]] ChainableResult<void> write_porymap_anim_key_frame(
        const ArtifactKey &dest_key, const Tileset &src, const std::string &anim_name) override;

    [[nodiscard]] ChainableResult<void>
    write_generated_anim_code(const ArtifactKey &dest_key, const Tileset &src) override;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] ChainableResult<void> write_bottom_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_middle_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_top_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_attributes_csv(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void>
    write_porytiles_pal_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index) override;

    [[nodiscard]] ChainableResult<void> write_porytiles_anim_frame(
        const ArtifactKey &dest_key,
        const Tileset &src,
        const std::string &anim_name,
        std::size_t frame_index) override;

    [[nodiscard]] ChainableResult<void> write_porytiles_anim_key_frame(
        const ArtifactKey &dest_key, const Tileset &src, const std::string &anim_name) override;

    [[nodiscard]] ChainableResult<void> write_anim_yaml(const ArtifactKey &dest_key, const Tileset &src) override;

  private:
    InfraConfig *config_;
    std::filesystem::path project_root_;
    std::filesystem::path transaction_root_;
    PngRgbaImageSaver *png_rgba_saver_;
    PngIndexedImageSaver *png_indexed_saver_;
    FilePalSaver *pal_saver_;
};

} // namespace porytiles2

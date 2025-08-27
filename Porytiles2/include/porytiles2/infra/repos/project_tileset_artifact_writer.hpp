#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/repos/tileset_artifact_writer.hpp"
#include "porytiles2/infra/config/infra_config.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"

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
        gsl::not_null<PngIndexedImageSaver *> png_indexed_saver)
        : config_{config}, project_root_{std::move(project_root)}, png_rgba_saver_{png_rgba_saver},
          png_indexed_saver_{png_indexed_saver}
    {
    }

    [[nodiscard]] Result<void> begin_transaction() override;

    [[nodiscard]] Result<void> commit() override;

    [[nodiscard]] Result<void> rollback() override;

    [[nodiscard]] Result<void>
    write(const ArtifactKey &dest_key, const TilesetArtifact &artifact, const Tileset &src) override;

  private:
    InfraConfig *config_;
    std::filesystem::path project_root_;
    std::filesystem::path transaction_root_;
    PngRgbaImageSaver *png_rgba_saver_;
    PngIndexedImageSaver *png_indexed_saver_;
};

} // namespace porytiles2

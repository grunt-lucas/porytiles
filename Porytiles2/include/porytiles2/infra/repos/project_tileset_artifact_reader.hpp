#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/infra/services/file_pal_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Provides a filesystem-based implementation for TilesetArtifactReader.
 *
 * @details
 * This class implements the TilesetArtifactReader interface to provide reading functionality for tileset artifacts. It
 * operates within the context of a Pokémon Gen III decompilation project on the local filesystem.
 */
class ProjectTilesetArtifactReader final : public TilesetArtifactReader {
  public:
    ProjectTilesetArtifactReader(
        gsl::not_null<const PngRgbaImageLoader *> png_rgba_loader,
        gsl::not_null<const PngIndexedImageLoader *> png_indexed_loader,
        gsl::not_null<const FilePalLoader *> pal_loader)
        : png_rgba_loader_{png_rgba_loader}, png_indexed_loader_{png_indexed_loader}, pal_loader_{pal_loader}
    {
    }

    [[nodiscard]] ChainableResult<void>
    read(Tileset &dest, const ArtifactKey &src_key, const TilesetArtifact &artifact) const override;

  private:
    const PngRgbaImageLoader *png_rgba_loader_;
    const PngIndexedImageLoader *png_indexed_loader_;
    const FilePalLoader *pal_loader_;
};

} // namespace porytiles2

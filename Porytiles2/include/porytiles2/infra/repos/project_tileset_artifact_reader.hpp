#pragma once

#include "gsl/pointers"

#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/infra/services/anim_code_parser.hpp"
#include "porytiles2/infra/services/anim_yaml_parser.hpp"
#include "porytiles2/infra/services/attributes_csv_loader.hpp"
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
        gsl::not_null<const FilePalLoader *> pal_loader,
        gsl::not_null<const AttributesCsvLoader *> attributes_csv_loader,
        gsl::not_null<const AnimYamlParser *> anim_yaml_parser,
        gsl::not_null<const AnimCodeParser *> anim_code_parser)
        : png_rgba_loader_{png_rgba_loader}, png_indexed_loader_{png_indexed_loader}, pal_loader_{pal_loader},
          attributes_csv_loader_{attributes_csv_loader}, anim_yaml_parser_{anim_yaml_parser},
          anim_code_parser_{anim_code_parser}
    {
    }

    /*
     * Porymap artifacts
     */
    [[nodiscard]] ChainableResult<void> read_metatiles_bin(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void>
    read_metatile_attributes_bin(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_tiles_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void>
    read_porymap_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const override;

    [[nodiscard]] ChainableResult<void> read_porymap_anim_frame(
        Tileset &dest,
        const ArtifactKey &src_key,
        const std::string &anim_name,
        std::size_t frame_index) const override;

    [[nodiscard]] ChainableResult<void>
    read_anim_code(Tileset &dest, const AnimationCallbackInfo &callback_info) const override;

    /*
     * Porytiles artifacts
     */
    [[nodiscard]] ChainableResult<void> read_bottom_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_middle_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_top_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_attributes_csv(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void>
    read_porytiles_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const override;

    [[nodiscard]] ChainableResult<void> read_porytiles_anim_frame(
        Tileset &dest,
        const ArtifactKey &src_key,
        const std::string &anim_name,
        std::size_t frame_index) const override;

    [[nodiscard]] ChainableResult<void> read_porytiles_anim_key_frame(
        Tileset &dest, const ArtifactKey &src_key, const std::string &anim_name) const override;

    [[nodiscard]] ChainableResult<void> read_anim_yaml(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_config(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_local_config(Tileset &dest, const ArtifactKey &src_key) const override;

  private:
    const PngRgbaImageLoader *png_rgba_loader_;
    const PngIndexedImageLoader *png_indexed_loader_;
    const FilePalLoader *pal_loader_;
    const AttributesCsvLoader *attributes_csv_loader_;
    const AnimYamlParser *anim_yaml_parser_;
    const AnimCodeParser *anim_code_parser_;
};

} // namespace porytiles2

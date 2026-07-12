#pragma once

#include <utility>

#include "gsl/pointers"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles/infra/services/anim_code_parser.hpp"
#include "porytiles/infra/services/anim_json_parser.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
#include "porytiles/infra/services/file_pal_loader.hpp"
#include "porytiles/infra/services/png_indexed_image_loader.hpp"
#include "porytiles/infra/services/png_rgba_image_loader.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/infra/services/tileset_attribute_schema_cache.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Provides a filesystem-based implementation for TilesetArtifactReader.
///
/// @details
/// This class implements the TilesetArtifactReader interface to provide reading functionality for tileset artifacts. It
/// operates within the context of a Pokémon Gen III decompilation project on the local filesystem.
///
/// Metatile attribute artifacts decode against the owning tileset's schema, looked up through the injected
/// TilesetAttributeSchemaCache by the destination tileset's name. This matters when a command reads a tileset other
/// than its target: compiling a secondary loads the paired primary's artifacts, and the primary can resolve a different
/// schema.
class ProjectTilesetArtifactReader final : public TilesetArtifactReader {
  public:
    ProjectTilesetArtifactReader(
        std::filesystem::path project_root,
        gsl::not_null<const TilesetAttributeSchemaCache *> schema_cache,
        gsl::not_null<const PngRgbaImageLoader *> png_rgba_loader,
        gsl::not_null<const PngIndexedImageLoader *> png_indexed_loader,
        gsl::not_null<const FilePalLoader *> pal_loader,
        gsl::not_null<const AttributesCsvLoader *> attributes_csv_loader,
        gsl::not_null<const AnimJsonParser *> anim_json_parser,
        gsl::not_null<const AnimCodeParser *> anim_code_parser,
        gsl::not_null<const ProjectTilesetMetadataProvider *> metadata_provider)
        : project_root_{std::move(project_root)}, schema_cache_{schema_cache}, png_rgba_loader_{png_rgba_loader},
          png_indexed_loader_{png_indexed_loader}, pal_loader_{pal_loader},
          attributes_csv_loader_{attributes_csv_loader}, anim_json_parser_{anim_json_parser},
          anim_code_parser_{anim_code_parser}, metadata_provider_{metadata_provider}
    {
    }

    // Porymap artifacts
    [[nodiscard]] ChainableResult<void> read_metatiles_bin(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void>
    read_metatile_attributes_bin(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_tiles_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void>
    read_porymap_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const override;

    [[nodiscard]] ChainableResult<void> read_porymap_anim(
        Tileset &dest,
        const std::string &anim_name,
        const ArtifactKey &params_key,
        const std::vector<std::pair<std::string, ArtifactKey>> &frame_keys) const override;

    // Porytiles artifacts
    [[nodiscard]] ChainableResult<void> read_bottom_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_middle_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_top_png(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void> read_attributes_csv(Tileset &dest, const ArtifactKey &src_key) const override;

    [[nodiscard]] ChainableResult<void>
    read_porytiles_pal_n(Tileset &dest, const ArtifactKey &src_key, std::size_t index) const override;

    [[nodiscard]] ChainableResult<void> read_porytiles_anim(
        Tileset &dest,
        const std::string &anim_name,
        const ArtifactKey &params_key,
        const std::optional<ArtifactKey> &key_frame_key,
        const std::vector<std::pair<std::string, ArtifactKey>> &frame_keys) const override;

    [[nodiscard]] ChainableResult<void>
    read_porytiles_primary_anim_references(Tileset &dest, const ArtifactKey &params_key) const override;

  private:
    const std::filesystem::path project_root_;
    const TilesetAttributeSchemaCache *schema_cache_;
    const PngRgbaImageLoader *png_rgba_loader_;
    const PngIndexedImageLoader *png_indexed_loader_;
    const FilePalLoader *pal_loader_;
    const AttributesCsvLoader *attributes_csv_loader_;
    const AnimJsonParser *anim_json_parser_;
    const AnimCodeParser *anim_code_parser_;
    const ProjectTilesetMetadataProvider *metadata_provider_;
};

} // namespace porytiles

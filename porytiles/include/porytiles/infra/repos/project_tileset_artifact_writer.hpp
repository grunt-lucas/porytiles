#pragma once

#include <map>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/config/domain_config.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/repos/tileset_artifact_writer.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/infra/config/infra_config.hpp"
#include "porytiles/infra/services/anim_code_generator.hpp"
#include "porytiles/infra/services/anim_json_parser.hpp"
#include "porytiles/infra/services/file_palette_saver.hpp"
#include "porytiles/infra/services/png_indexed_image_saver.hpp"
#include "porytiles/infra/services/png_rgba_image_saver.hpp"
#include "porytiles/infra/services/project_tileset_metadata_provider.hpp"
#include "porytiles/infra/services/project_tileset_metadata_writer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Provides a filesystem-based implementation for TilesetArtifactWriter.
///
/// @details
/// This class implements the TilesetArtifactWriter interface to provide writing functionality for tileset artifacts. It
/// operates within the context of a Pokémon Gen III decompilation project on the local filesystem.
class ProjectTilesetArtifactWriter final : public TilesetArtifactWriter {
  public:
    ProjectTilesetArtifactWriter(
        gsl::not_null<DomainConfig *> domain_config,
        gsl::not_null<InfraConfig *> infra_config,
        std::filesystem::path project_root,
        gsl::not_null<const Schema *> schema,
        gsl::not_null<const ProviderMap *> providers,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag,
        gsl::not_null<PngRgbaImageSaver *> png_rgba_saver,
        gsl::not_null<PngIndexedImageSaver *> png_indexed_saver,
        gsl::not_null<FilePaletteSaver *> palette_saver,
        gsl::not_null<const AnimJsonParser *> anim_json_parser,
        gsl::not_null<const AnimCodeGenerator *> anim_code_generator)
        : domain_config_{domain_config}, infra_config_{infra_config}, project_root_{std::move(project_root)},
          schema_{schema}, providers_{providers}, format_{format}, diag_{diag}, png_rgba_saver_{png_rgba_saver},
          png_indexed_saver_{png_indexed_saver}, palette_saver_{palette_saver}, anim_json_parser_{anim_json_parser},
          anim_code_generator_{anim_code_generator}, metadata_provider_{project_root_, format, diag},
          metadata_writer_{project_root_, format}
    {
    }

    [[nodiscard]] ChainableResult<void> begin_transaction() override;

    [[nodiscard]] ChainableResult<void> commit() override;

    [[nodiscard]] ChainableResult<void> rollback() override;

    // Porymap artifacts
    [[nodiscard]] ChainableResult<void> write_metatiles_bin(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void>
    write_metatile_attributes_bin(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_tiles_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void>
    write_porymap_palette_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index) override;

    [[nodiscard]] ChainableResult<void> write_porymap_anim_frame(
        const ArtifactKey &dest_key,
        const Tileset &src,
        const std::string &anim_name,
        const std::string &frame_name) override;

    [[nodiscard]] ChainableResult<void>
    write_porymap_anim_params(const ArtifactKey &dest_key, const Tileset &src) override;

    // Porytiles artifacts
    [[nodiscard]] ChainableResult<void> write_bottom_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_middle_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_top_png(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void> write_attributes_csv(const ArtifactKey &dest_key, const Tileset &src) override;

    [[nodiscard]] ChainableResult<void>
    write_porytiles_palette_n(const ArtifactKey &dest_key, const Tileset &src, std::size_t index) override;

    [[nodiscard]] ChainableResult<void> write_porytiles_anim_frame(
        const ArtifactKey &dest_key,
        const Tileset &src,
        const std::string &anim_name,
        const std::string &frame_name) override;

    [[nodiscard]] ChainableResult<void>
    write_porytiles_anim_params(const ArtifactKey &dest_key, const Tileset &src) override;

  private:
    /// @brief Represents a staged directory to be atomically moved during commit.
    ///
    /// @details
    /// Each StagedDirectory tracks a directory in the staging area that will be
    /// atomically moved to its final destination via std::filesystem::rename().
    struct StagedDirectory {
        std::filesystem::path staging_path; ///< Path in the transaction tmpdir
        std::filesystem::path dest_path;    ///< Final destination path
    };

    DomainConfig *domain_config_;
    InfraConfig *infra_config_;
    std::filesystem::path project_root_;
    const Schema *schema_;
    const ProviderMap *providers_;
    std::filesystem::path transaction_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    PngRgbaImageSaver *png_rgba_saver_;
    PngIndexedImageSaver *png_indexed_saver_;
    FilePaletteSaver *palette_saver_;
    const AnimJsonParser *anim_json_parser_;
    const AnimCodeGenerator *anim_code_generator_;
    ProjectTilesetMetadataProvider metadata_provider_;
    ProjectTilesetMetadataWriter metadata_writer_;

    /// Map from destination directory path to its staging info
    std::map<std::filesystem::path, StagedDirectory> staged_directories_;
    /// List of (staging_path, dest_path) pairs for special files handled individually
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> staged_special_files_;
};

} // namespace porytiles

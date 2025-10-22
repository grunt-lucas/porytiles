#include "porytiles2/di/components.hpp"

#include <filesystem>
#include <functional>

#include "fruit/fruit.h"

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/color_set_builder.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles2/infra/services/file_pal_saver.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"
#include "porytiles2/infra/services/stderr_ascii_tile_printer.hpp"
#include "porytiles2/utilities/text/ansi_styled_text_formatter.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"

namespace porytiles2::di {

// ============================================================================
// CONDITIONAL FORMATTER COMPONENT
// ============================================================================

fruit::Component<TextFormatter> getFormatterComponent(const ColorConfig &config)
{
    if (config.should_use_color()) {
        return fruit::createComponent()
            .bind<TextFormatter, AnsiStyledTextFormatter>()
            .registerConstructor<AnsiStyledTextFormatter()>();
    }
    return fruit::createComponent()
        .bind<TextFormatter, PlainTextFormatter>()
        .registerConstructor<PlainTextFormatter()>();
}

// ============================================================================
// FOUNDATION COMPONENTS
// ============================================================================

fruit::Component<fruit::Required<TextFormatter>, UserDiagnostics, TilePrinter> getFoundationComponent()
{
    return fruit::createComponent()
        .bind<UserDiagnostics, StderrStyledUserDiagnostics>()
        .bind<TilePrinter, StderrAsciiTilePrinter>()
        .registerConstructor<StderrStyledUserDiagnostics(TextFormatter *)>()
        .registerConstructor<StderrAsciiTilePrinter(TextFormatter *)>();
}

// ============================================================================
// CONFIG COMPONENT
// ============================================================================

fruit::Component<fruit::Required<TextFormatter>, DomainConfig> getConfigComponent()
{
    return fruit::createComponent()
        .bind<DomainConfig, LazyLayeredConfig>()
        .registerProvider<LazyLayeredConfig *(TextFormatter *)>([](TextFormatter *formatter) -> LazyLayeredConfig * {
            std::vector<std::unique_ptr<ConfigProvider>> providers;
            providers.push_back(std::make_unique<DefaultProvider>());
            // Fruit will manage the lifetime of this pointer
            return new LazyLayeredConfig(formatter, std::move(providers));
        });
}

// ============================================================================
// LOADER SERVICES COMPONENT
// ============================================================================

fruit::Component<PngRgbaImageLoader, PngIndexedImageLoader, JascPalLoader> getLoaderServicesComponent()
{
    return fruit::createComponent()
        .registerConstructor<PngRgbaImageLoader()>()
        .registerConstructor<PngIndexedImageLoader()>()
        .registerConstructor<JascPalLoader()>();
}

// ============================================================================
// DOMAIN SERVICES COMPONENT
// ============================================================================

fruit::Component<
    fruit::Required<DomainConfig, TextFormatter, UserDiagnostics, TilePrinter>,
    PrimaryTilesetCompiler,
    TileValidator,
    ColorSetBuilder>
getDomainServicesComponent()
{
    return fruit::createComponent()
        .registerConstructor<PrimaryTilesetCompiler(
            DomainConfig *, TextFormatter *, UserDiagnostics *, TilePrinter *)>()
        .registerConstructor<TileValidator(TextFormatter *, UserDiagnostics *, TilePrinter *)>()
        .registerConstructor<ColorSetBuilder(TextFormatter *)>();
}

// ============================================================================
// REPOSITORY COMPONENT
// ============================================================================

fruit::Component<fruit::Required<DomainConfig>, TilesetRepo> getRepositoryComponent()
{
    return fruit::createComponent()
        // Register provider for ProjectTilesetArtifactKeyProvider with hardcoded project_root for now
        .registerProvider<ProjectTilesetArtifactKeyProvider *()>([]() -> ProjectTilesetArtifactKeyProvider * {
            // Fruit will manage the lifetime of this pointer
            return new ProjectTilesetArtifactKeyProvider(std::filesystem::path("."));
        })
        .bind<ArtifactChecksumProvider, ProjectArtifactChecksumProvider>()
        .registerConstructor<ProjectArtifactChecksumProvider(ProjectTilesetArtifactKeyProvider *)>()
        .registerConstructor<ProjectTilesetArtifactReader(
            PngRgbaImageLoader *, PngIndexedImageLoader *, JascPalLoader *)>()
        // Register saver services
        .registerConstructor<PngRgbaImageSaver()>()
        .registerConstructor<PngIndexedImageSaver()>()
        .bind<FilePalSaver, JascPalSaver>()
        .registerConstructor<JascPalSaver()>()
        // Register ProjectTilesetArtifactWriter with provider to handle InfraConfig and path
        .registerProvider<ProjectTilesetArtifactWriter *(
            DomainConfig *, PngRgbaImageSaver *, PngIndexedImageSaver *, FilePalSaver *)>(
            [](DomainConfig *config,
               PngRgbaImageSaver *png_rgba_saver,
               PngIndexedImageSaver *png_indexed_saver,
               FilePalSaver *pal_saver) -> ProjectTilesetArtifactWriter * {
                // Cast DomainConfig to InfraConfig (safe since LazyLayeredConfig implements both)
                auto *infra_config = dynamic_cast<InfraConfig *>(config);
                // Fruit will manage the lifetime of this pointer
                return new ProjectTilesetArtifactWriter(
                    infra_config, std::filesystem::path("."), png_rgba_saver, png_indexed_saver, pal_saver);
            })
        .registerConstructor<TilesetRepo(
            ArtifactChecksumProvider *,
            ProjectTilesetArtifactKeyProvider *,
            ProjectTilesetArtifactReader *,
            ProjectTilesetArtifactWriter *)>()
        .install(getLoaderServicesComponent);
}

// ============================================================================
// APPLICATION COMPONENT (COMPOSITION ROOT)
// ============================================================================

fruit::Component<PrimaryTilesetCompiler, TilesetRepo, TileValidator, UserDiagnostics, TextFormatter>
getApplicationComponent(const ColorConfig &config)
{
    // Directly inline the formatter selection here
    if (config.should_use_color()) {
        return fruit::createComponent()
            .bind<TextFormatter, AnsiStyledTextFormatter>()
            .registerConstructor<AnsiStyledTextFormatter()>()
            .install(getFoundationComponent)
            .install(getConfigComponent)
            .install(getDomainServicesComponent)
            .install(getRepositoryComponent);
    }
    return fruit::createComponent()
        .bind<TextFormatter, PlainTextFormatter>()
        .registerConstructor<PlainTextFormatter()>()
        .install(getFoundationComponent)
        .install(getConfigComponent)
        .install(getDomainServicesComponent)
        .install(getRepositoryComponent);
}

} // namespace porytiles2::di
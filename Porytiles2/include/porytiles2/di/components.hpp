#pragma once

#include "fruit/fruit.h"

#include "porytiles2/di/color_config.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/color_set_builder.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/domain/services/tile_validator.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2::di {

// ============================================================================
// CONDITIONAL FORMATTER COMPONENT
// ============================================================================

/**
 * @brief Component that provides TextFormatter based on runtime configuration.
 *
 * @details
 * This component conditionally binds either AnsiStyledTextFormatter or PlainTextFormatter based on the ColorConfig
 * configuration. This is the primary example of runtime conditional injection.
 *
 * @param config Runtime configuration containing color preferences and TTY detection
 * @return Component providing TextFormatter interface
 */
fruit::Component<TextFormatter> getFormatterComponent(const ColorConfig &config);

// ============================================================================
// FOUNDATION COMPONENTS
// ============================================================================

/**
 * @brief Component providing foundation diagnostic and printing services.
 *
 * @details
 * This component provides UserDiagnostics and TilePrinter implementations. Both depend on TextFormatter, which must be
 * provided by another component.
 *
 * @return Component providing UserDiagnostics and TilePrinter, requiring TextFormatter
 */
fruit::Component<fruit::Required<TextFormatter>, UserDiagnostics, TilePrinter> getFoundationComponent();

// ============================================================================
// CONFIG COMPONENT
// ============================================================================

/**
 * @brief Component providing DomainConfig implementation.
 *
 * @details
 * Provides the LazyLayeredConfig implementation of DomainConfig.
 *
 * @return Component providing DomainConfig
 */
fruit::Component<fruit::Required<TextFormatter>, DomainConfig> getConfigComponent();

// ============================================================================
// LOADER SERVICES COMPONENT
// ============================================================================

/**
 * @brief Component providing stateless image and palette loader services.
 *
 * @details
 * These loaders are stateless and can be created as needed.
 *
 * @return Component providing all loader services
 */
fruit::Component<PngRgbaImageLoader, PngIndexedImageLoader, JascPalLoader> getLoaderServicesComponent();

// ============================================================================
// DOMAIN SERVICES COMPONENT
// ============================================================================

/**
 * @brief Component providing primary domain services.
 *
 * @details
 * Provides compiler and validator services. These depend on foundation services and configuration, which must be
 * provided by other components.
 *
 * @return Component providing domain services with their requirements
 */
fruit::Component<
    fruit::Required<DomainConfig, TextFormatter, UserDiagnostics, TilePrinter>,
    PrimaryTilesetCompiler,
    TileValidator,
    ColorSetBuilder>
getDomainServicesComponent();

// ============================================================================
// REPOSITORY COMPONENT
// ============================================================================

/**
 * @brief Component providing repository infrastructure.
 *
 * @details
 * Provides TilesetRepo and all its dependencies including artifact readers, writers, and checksum providers.
 *
 * @return Component providing repository infrastructure
 */
fruit::Component<fruit::Required<DomainConfig>, TilesetRepo> getRepositoryComponent();

// ============================================================================
// APPLICATION COMPONENT (COMPOSITION ROOT)
// ============================================================================

/**
 * @brief Main application component that composes all sub-components.
 *
 * @details
 * This is the composition root that assembles all components based on runtime configuration. It handles conditional
 * injection of TextFormatter and wires all dependencies together.
 *
 * Usage:
 * ```C++
 * ColorConfig config{.no_color = false, .tileset_name = "my_tileset"};
 * fruit::Injector<PrimaryTilesetCompiler, TilesetRepo> injector(
 *     getApplicationComponent, config);
 * auto* compiler = injector.get<PrimaryTilesetCompiler*>();
 * ```
 *
 * @param config Runtime configuration determining component bindings
 * @return Component providing all application services
 */
fruit::Component<PrimaryTilesetCompiler, TilesetRepo, TileValidator, UserDiagnostics, TextFormatter>
getApplicationComponent(const ColorConfig &config);

} // namespace porytiles2::di
#pragma once

#include <memory>
#include <string>

#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles2/di/components.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/layer_image_metatileizer.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/domain/services/tile_printer.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_reader.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles2/infra/services/jasc_pal_loader.hpp"
#include "porytiles2/infra/services/jasc_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_loader.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_loader.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/infra/services/project_artifact_checksum_provider.hpp"
#include "porytiles2/infra/services/stderr_ascii_tile_printer.hpp"
#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

#include "command.hpp"

class DebugPrimaryCompileCommand final : public Command {
  public:
    explicit DebugPrimaryCompileCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to compile")->required();
    }

    void Run() override
    {
        using namespace porytiles2;

        /*
         * TODO: once we have more compilation code finished, we should come back and do more dependency injection via
         * Fruit.
         */
        // Use Fruit DI to inject TextFormatter based on no_color flag
        const bool no_color = !isatty(STDERR_FILENO); // Disable color when stderr is not a terminal
        fruit::Injector injector{di::get_formatter_component, no_color};
        auto text_formatter = injector.get<TextFormatter *>();

        // Manually create other services (not yet using DI for these)
        std::unique_ptr<UserDiagnostics> diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter);
        std::unique_ptr<TilePrinter> tile_printer = std::make_unique<StderrAsciiTilePrinter>(text_formatter);

        // Setup layered configuration
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{text_formatter, std::move(providers)};

        // Initialize stateless services
        PngRgbaImageLoader png_rgba_loader{};
        PngIndexedImageLoader png_indexed_loader{};
        PngRgbaImageSaver png_rgba_saver{};
        PngIndexedImageSaver png_indexed_saver{};
        JascPalLoader jasc_loader{};
        JascPalSaver jasc_saver{};

        // Setup primary compiler
        PrimaryTilesetCompiler compiler{&config, text_formatter, diag.get(), tile_printer.get()};

        // Setup the tileset repository
        ProjectTilesetArtifactReader artifact_reader{&png_rgba_loader, &png_indexed_loader, &jasc_loader};
        ProjectTilesetArtifactWriter artifact_writer{&config, ".", &png_rgba_saver, &png_indexed_saver, &jasc_saver};
        ProjectTilesetArtifactKeyProvider key_provider{"."};
        ProjectArtifactChecksumProvider checksum_provider{&key_provider};
        TilesetRepo repo{&checksum_provider, &key_provider, &artifact_reader, &artifact_writer};

        // Test the UserDiagnostics interface
        // diag->note("this is a test note");
        // diag->warn("test-warning", "this is a test warning");
        // diag->warn_note("test-warning", "this is a test warning note");
        // diag->err("this is a test error");

        // Load the tileset
        auto maybe_tileset = repo.load(tileset_name_);
        if (!maybe_tileset.has_value()) {
            diag->fatal(maybe_tileset);
            return;
        }
        const auto tileset = std::move(maybe_tileset.value());

        // Compile the tileset
        auto compile_result = compiler.compile_patch_tiles_fixed_pals_fixed(*tileset);
        if (!compile_result.has_value()) {
            const auto fail_result = ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to compile tileset '{}'", FormatParam{tileset_name_, Style::bold}},
                compile_result};
            diag->fatal(fail_result);
            return;
        }
        const auto new_tileset = std::move(compile_result.value());

        // Multi-line fatal print demo
        // FormattableError proximate{std::vector<std::string>{
        //     "this is line 1 of the error", "this is line 2 of the error", "this is line 3 of the error"}};
        // FormattableError middle{std::vector<std::string>{
        //     "this is line 1 of the error", "this is line 2 of the error", "this is line 3 of the error"}};
        // FormattableError root{std::vector<std::string>{
        //     "this is line 1 of the error", "this is line 2 of the error", "this is line 3 of the error"}};
        // ChainableResult<void> root_result{root};
        // ChainableResult<void> middle_result{middle, root_result};
        // ChainableResult<void> prox_result{proximate, middle_result};
        // diag->fatal(prox_result);

        // Save the tileset back
        const auto new_tileset_save_result = repo.save(*new_tileset);
        if (!new_tileset_save_result.has_value()) {
            diag->fatal(new_tileset_save_result);
            return;
        }
    }

  private:
    static constexpr auto kCommandName = "debug-compile-primary";
    static constexpr auto kCommandDesc =
        "Load a tileset, run it through the compile-primary service, and write it back.";
    static constexpr auto kCommandGroup = "COMMANDS";
    std::string tileset_name_;
};

#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

#include "CLI/CLI.hpp"
#include "fruit/fruit.h"

#include "porytiles2/utilities/result/chainable_result.hpp"

#include "porytiles2/infra/cli/cli_option_registration.hpp"
#include "porytiles2/infra/cli/cli_option_storage.hpp"
#include "porytiles2/infra/config/cli_option_provider.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/header_define_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/config/metatiles_header_provider.hpp"
#include "porytiles2/infra/config/yaml_file_provider.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/di/components.hpp"
#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

#include "command.hpp"
#include "option.hpp"

class DumpTilesetConfigCommand final : public Command {
  public:
    explicit DumpTilesetConfigCommand(CLI::App &parent_app)
        : Command{parent_app, kCommandName, kCommandDesc, kCommandGroup}
    {
        CLI::App &cmd = get_app();
        cmd.add_option("<tileset-name>", tileset_name_, "Name of the tileset to dump config for")->required();
        project_root_opt_.RegisterOpt(cmd);
        porytiles2::register_config_options(cmd, cli_storage_);
    }

    void Run() override
    {
        using namespace porytiles2;

        // Use Fruit DI to inject TextFormatter based on no_color flag
        const bool no_color = !isatty(STDERR_FILENO);
        fruit::Injector injector{di::get_formatter_component, no_color};
        auto text_formatter = injector.get<TextFormatter *>();

        // Create unfiltered diag for config bootstrapping (so config-loading warnings always show)
        auto stderr_diag = std::make_unique<StderrStyledUserDiagnostics>(text_formatter);

        std::filesystem::path project_root = project_root_opt_.project_root();
        std::filesystem::path fieldmap_header_root_relative{"include/fieldmap.h"};

        // Setup layered configuration (CLI options have highest priority)
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<CliOptionProvider>(cli_storage_));
        auto yaml_provider = std::make_unique<YamlFileProvider>(text_formatter, stderr_diag.get(), project_root);
        auto *yaml_provider_ptr = yaml_provider.get();
        providers.push_back(std::move(yaml_provider));
        providers.push_back(
            std::make_unique<HeaderDefineProvider>(project_root, fieldmap_header_root_relative, text_formatter));
        providers.push_back(std::make_unique<MetatilesHeaderProvider>(project_root, text_formatter));
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{text_formatter, std::move(providers)};

        // Eagerly validate all YAML config files for unknown keys
        if (yaml_provider_ptr->preload_and_validate(ConfigScopeType::tileset, tileset_name_)) {
            const auto validation_err = ChainableResult<void>{FormattableError{
                "Configuration validation failed for tileset '{}'.", FormatParam{tileset_name_, Style::bold}}};
            stderr_diag->fatal(validation_err);
            throw CLI::RuntimeError{1};
        }

        config.dump_config(std::cout, ConfigScopeType::tileset, tileset_name_);
    }

  private:
    static constexpr auto kCommandName = "dump-tileset-config";
    static constexpr auto kCommandDesc = "Dump the full configuration provenance chain for a tileset.";
    static constexpr auto kCommandGroup = "UTILITIES";
    std::string tileset_name_;
    OptProjectRoot project_root_opt_;
    porytiles2::CliOptionStorage cli_storage_;
};

#pragma once

#include <string>
#include <unordered_set>

#include <CLI/CLI.hpp>

#include <porytiles/diagnostics/diagnostics.hpp>

#include "./option.hpp"

class OptGroup {
  public:
    virtual ~OptGroup() = default;
    virtual std::string GroupName() = 0;
    virtual void RegisterGroup(CLI::App &app) = 0;
};

class OptGroupFieldmap final : public OptGroup {
    std::string base_game_preset_;
    std::size_t tiles_primary_override_;
    std::size_t tiles_total_override_;
    std::size_t metatiles_primary_override_;
    std::size_t metatiles_total_override_;
    std::size_t pals_primary_override_;
    std::size_t pals_total_override_;

  public:
    OptGroupFieldmap()
        // TODO : 'pokeemerald' base game string should be defined by an enum, like TilesPalMode
        : base_game_preset_{"pokeemerald"}, tiles_primary_override_{0}, tiles_total_override_{0},
          metatiles_primary_override_{0}, metatiles_total_override_{0}, pals_primary_override_{0},
          pals_total_override_{0} {}

    [[nodiscard]] std::string GroupName() override {
        return "FIELDMAP OVERRIDE OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override {
        // TODO : create a base game validator
        app.add_option("--base-game-preset", base_game_preset_, "Base game preset to use for the tileset.")
            ->group(GroupName())
            ->capture_default_str();
        app.add_option("--tiles-primary-override", tiles_primary_override_,
                       "Override the number of tiles in the primary tileset.")
            ->group(GroupName());
        app.add_option("--tiles-total-override", tiles_total_override_,
                       "Override the total number of tiles in the tileset.")
            ->group(GroupName());
        app.add_option("--metatiles-primary-override", metatiles_primary_override_,
                       "Override the number of metatiles in the primary tileset.")
            ->group(GroupName());
        app.add_option("--metatiles-total-override", metatiles_total_override_,
                       "Override the total number of metatiles in the tileset.")
            ->group(GroupName());
        app.add_option("--pals-primary-override", pals_primary_override_,
                       "Override the number of metatiles in the primary tileset.")
            ->group(GroupName());
        app.add_option("--pals-total-override", pals_total_override_,
                       "Override the total number of metatiles in the tileset.")
            ->group(GroupName());
    }

    [[nodiscard]] const std::string &base_game_preset() const {
        return base_game_preset_;
    }
};

class OptGroupDiagnostics final : public OptGroup {
    std::vector<std::string> diagnostics_;

  public:
    OptGroupDiagnostics() = default;

    [[nodiscard]] std::string GroupName() override {
        return "DIAGNOSTIC OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override {
        app.add_option("--warning", diagnostics_, "Enable given warning diagnostics.")
            ->check(DiagnosticIsWarningValidator{})
            ->group(GroupName());
        app.add_option("--no-warning", diagnostics_, "Disable given warning diagnostics.")
            ->check(DiagnosticIsWarningValidator{})
            ->group(GroupName());
        app.add_option("--error", diagnostics_, "Enable given warning diagnostics as errors.")
            ->check(DiagnosticIsWarningValidator{})
            ->group(GroupName());
        app.add_option("--no-error", diagnostics_, "Disable given warning diagnostics as errors.")
            ->check(DiagnosticIsWarningValidator{})
            ->group(GroupName());
    }

    [[nodiscard]] const std::vector<std::string> &diagnostics() const {
        return diagnostics_;
    }
};

class OptGroupArtifacts final : public OptGroup {
    OptOutput output_opt_;
    OptTilesPalMode tiles_pal_mode_opt_;
    OptDisableMetatileGeneration disable_metatile_generation_opt_;
    OptDisableAttributeGeneration disable_attribute_generation_opt_;

  public:
    OptGroupArtifacts() = default;

    [[nodiscard]] std::string GroupName() override {
        return "ARTIFACT OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override {
        output_opt_.RegisterOpt(app);
        output_opt_.SetGroup(GroupName(), app);
        tiles_pal_mode_opt_.RegisterOpt(app);
        tiles_pal_mode_opt_.SetGroup(GroupName(), app);
        disable_metatile_generation_opt_.RegisterOpt(app);
        disable_metatile_generation_opt_.SetGroup(GroupName(), app);
        disable_attribute_generation_opt_.RegisterOpt(app);
        disable_attribute_generation_opt_.SetGroup(GroupName(), app);
    }

    [[nodiscard]] const OptOutput &output_opt() const {
        return output_opt_;
    }

    [[nodiscard]] const OptTilesPalMode &tiles_pal_mode() const {
        return tiles_pal_mode_opt_;
    }

    [[nodiscard]] bool metatiles_disabled() const {
        return disable_metatile_generation_opt_.disabled();
    }

    [[nodiscard]] bool attributes_disabled() const {
        return disable_attribute_generation_opt_.disabled();
    }
};

class OptGroupPalAssignmentConfig final : public OptGroup {
  public:
    OptGroupPalAssignmentConfig() = default;

    [[nodiscard]] std::string GroupName() override {
        return "PAL ASSIGNMENT CONFIG OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override {}
};

#pragma once

#include <string>
#include <unordered_set>

#include <CLI/CLI.hpp>

#include <porytiles/diagnostics/diagnostics.hpp>

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
        // TODO : 'pokeemerald' string should be defined by an enum, like TilesPalMode
        : base_game_preset_{"pokeemerald"}, tiles_primary_override_{0}, tiles_total_override_{0},
          metatiles_primary_override_{0}, metatiles_total_override_{0}, pals_primary_override_{0},
          pals_total_override_{0} {}

    [[nodiscard]] std::string GroupName() override {
        return "FIELDMAP OVERRIDE OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override {
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
    class DiagnosticIsWarningValidator final : public CLI::Validator {
      public:
        explicit DiagnosticIsWarningValidator(std::string hint) : Validator{std::move(hint)} {
            std::unordered_set<std::string> warning_diags;
            for (const auto name : porytiles::AllDiagTemplNames(porytiles::DiagLevel::Warning)) {
                warning_diags.insert(name);
            }
            name_ = "DIAGNOSTIC_IS_WARNING";
            func_ = [warning_diags](const std::string &str) {
                if (warning_diags.contains(str)) {
                    return std::string{};
                }
                return std::string{"invalid warning diagnostic: " + str};
            };
        }
    };

    std::vector<std::string> diagnostics_;

  public:
    OptGroupDiagnostics() = default;

    [[nodiscard]] std::string GroupName() override {
        return "DIAGNOSTIC OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override {
        app.add_option("--warning", diagnostics_, "Enable given warning diagnostics.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(GroupName());
        app.add_option("--no-warning", diagnostics_, "Disable given warning diagnostics.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(GroupName());
        app.add_option("--error", diagnostics_, "Enable given warning diagnostics as errors.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(GroupName());
        app.add_option("--no-error", diagnostics_, "Disable given warning diagnostics as errors.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(GroupName());
    }

    [[nodiscard]] const std::vector<std::string> &diagnostics() const {
        return diagnostics_;
    }
};

class OptGroupArtifacts final : public OptGroup {
    OptOutput output_opt_;
    OptTilesPalMode tiles_output_pal_opt_;

  public:
    OptGroupArtifacts() = default;

    [[nodiscard]] std::string GroupName() override {
        return "ARTIFACT OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override {
        output_opt_.RegisterOpt(app);
        output_opt_.SetGroup(GroupName(), app);
        tiles_output_pal_opt_.RegisterOpt(app);
        tiles_output_pal_opt_.SetGroup(GroupName(), app);
    }

    [[nodiscard]] const OptOutput &output_opt() const {
        return output_opt_;
    }
};

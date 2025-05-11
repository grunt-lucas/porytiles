#pragma once

#include <string>
#include <unordered_set>

#include <CLI/CLI.hpp>

#include <porytiles/diagnostics/diagnostics.hpp>
#include <porytiles/tiles/tiles_pal_mode.hpp>

class NotAlreadyAFileValidator final : public CLI::Validator {
  public:
    explicit NotAlreadyAFileValidator(std::string hint) : Validator{std::move(hint)} {
        name_ = "NOT_ALREADY_A_FILE";
        func_ = [](const std::string &str) {
            if (std::filesystem::exists(str) && std::filesystem::is_regular_file(str)) {
                return std::string{"file already exists: " + str};
            }
            return std::string{};
        };
    }
};

class OptGroup {
  public:
    virtual ~OptGroup() = default;
    virtual void RegisterOptions(CLI::App &app) = 0;
};

class OptGroupFieldmap final : public OptGroup {
    static constexpr auto kGroupName = "FIELDMAP OVERRIDE OPTIONS";

  public:
    std::string base_game_preset_;
    std::size_t tiles_primary_override_;
    std::size_t tiles_total_override_;
    std::size_t metatiles_primary_override_;
    std::size_t metatiles_total_override_;
    std::size_t pals_primary_override_;
    std::size_t pals_total_override_;

    void RegisterOptions(CLI::App &app) override {
        app.add_option("--base-game-preset", base_game_preset_, "Base game preset to use for the tileset.")
            ->group(kGroupName);
        app.add_option("--tiles-primary-override", tiles_primary_override_,
                       "Override the number of tiles in the primary tileset.")
            ->group(kGroupName);
        app.add_option("--tiles-total-override", tiles_total_override_,
                       "Override the total number of tiles in the tileset.")
            ->group(kGroupName);
        app.add_option("--metatiles-primary-override", metatiles_primary_override_,
                       "Override the number of metatiles in the primary tileset.")
            ->group(kGroupName);
        app.add_option("--metatiles-total-override", metatiles_total_override_,
                       "Override the total number of metatiles in the tileset.")
            ->group(kGroupName);
        app.add_option("--pals-primary-override", pals_primary_override_,
                       "Override the number of metatiles in the primary tileset.")
            ->group(kGroupName);
        app.add_option("--pals-total-override", pals_total_override_,
                       "Override the total number of metatiles in the tileset.")
            ->group(kGroupName);
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
            name_ = "IS_WARNING_DIAGNOSTIC";
            func_ = [warning_diags](const std::string &str) {
                if (warning_diags.contains(str)) {
                    return std::string{};
                }
                return std::string{"invalid warning diagnostic: " + str};
            };
        }
    };

    static constexpr auto kGroupName = "DIAGNOSTIC OPTIONS";

  public:
    std::vector<std::string> diagnostics_;

    void RegisterOptions(CLI::App &app) override {
        app.add_option("-W,--warning", diagnostics_, "Enable given warning diagnostic.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(kGroupName);
        app.add_option("--Wno,--no-warning", diagnostics_, "Disable given warning diagnostic.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(kGroupName);
        app.add_option("--Werror", diagnostics_, "Enable given warning diagnostic as error.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(kGroupName);
        app.add_option("--Wno-error", diagnostics_, "Disable given warning diagnostic as error.")
            ->check(DiagnosticIsWarningValidator{"DIAG"})
            ->group(kGroupName);
    }
};

class OptOutput final : public OptGroup {
    std::string output_path_;

  public:
    OptOutput() : output_path_{"."} {}

    void RegisterOptions(CLI::App &app) override {
        app.add_option("-o,--output", output_path_,
                       "Output generated files to the directory specified by PATH. If any element of PATH does not "
                       "exist, it will be created.")
            ->check(NotAlreadyAFileValidator{"PATH"})
            ->capture_default_str();
    }

    [[nodiscard]] std::string output_path() const {
        return output_path_;
    }
};

class OptTilesPalMode final : public OptGroup {
    class TilesPalModeValidator final : public CLI::Validator {
      public:
        explicit TilesPalModeValidator(std::string hint) : Validator{std::move(hint)} {
            name_ = "TILES_OUTPUT_PAL";
            func_ = [](const std::string &str) {
                if (!porytiles::TilesPalModeFromStr(str).has_value()) {
                    return std::string{"invalid 'tiles.png' output palette mode: " + str};
                }
                return std::string{};
            };
        }
    };

    std::string pal_format_;

  public:
    OptTilesPalMode() : pal_format_{porytiles::TilesPalModeToStr(porytiles::TilesPalMode::kTrueColor)} {}

    void RegisterOptions(CLI::App &app) override {
        app.add_option(
               "--tiles-pal-mode", pal_format_,
               "Set the palette mode for the output 'tiles.png'. Valid settings are 'true-color' or 'greyscale'. These "
               "settings are for human visual purposes only and have no effect on the final in-game tiles.")
            ->check(TilesPalModeValidator{"MODE"})
            ->capture_default_str();
    }

    [[nodiscard]] std::string pal_format() const {
        return pal_format_;
    }
};

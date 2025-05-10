#pragma once

#include <string>

#include <CLI/CLI.hpp>

class NotAlreadyFileValidator : public CLI::Validator {
  public:
    explicit NotAlreadyFileValidator(std::string description) : Validator{std::move(description)} {
        name_ = "NOT_ALREADY_FILE";
        non_modifying_ = true;
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
        app.add_option("--base-game-preset", base_game_preset_, "Base game preset to use for the tileset")
            ->group(kGroupName);
        app.add_option("--tiles-primary-override", tiles_primary_override_,
                       "Override the number of tiles in the primary tileset")
            ->group(kGroupName);
        app.add_option("--tiles-total-override", tiles_total_override_,
                       "Override the total number of tiles in the tileset")
            ->group(kGroupName);
        app.add_option("--metatiles-primary-override", metatiles_primary_override_,
                       "Override the number of metatiles in the primary tileset")
            ->group(kGroupName);
        app.add_option("--metatiles-total-override", metatiles_total_override_,
                       "Override the total number of metatiles in the tileset")
            ->group(kGroupName);
        app.add_option("--pals-primary-override", pals_primary_override_,
                       "Override the number of metatiles in the primary tileset")
            ->group(kGroupName);
        app.add_option("--pals-total-override", pals_total_override_,
                       "Override the total number of metatiles in the tileset")
            ->group(kGroupName);
    }
};

class OptGroupDiagnostics final : public OptGroup {
    static constexpr auto kGroupName = "DIAGNOSTIC OPTIONS";

  public:
    std::vector<std::string> diagnostics_;

    void RegisterOptions(CLI::App &app) override {
        app.add_option("--W", diagnostics_, "Enable given warning diagnostic")->group(kGroupName);
        app.add_option("--Wno", diagnostics_, "Disable given warning diagnostic")->group(kGroupName);
        app.add_option("--Werror", diagnostics_, "Enable given warning diagnostic as error")->group(kGroupName);
        app.add_option("--Wno-error", diagnostics_, "Disable given warning diagnostic as error")->group(kGroupName);
    }
};

class OptGroupOutput final : public OptGroup {
  public:
    std::string output_path_;

    OptGroupOutput() : output_path_{"."} {}

    void RegisterOptions(CLI::App &app) override {
        app.add_option("-o,--output", output_path_,
                       "Output generated files to the directory specified by PATH. If any element of PATH does not "
                       "exist, it will be created.")
            ->check(NotAlreadyFileValidator{"PATH"})
            ->capture_default_str();
    }
};

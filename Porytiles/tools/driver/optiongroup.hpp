#pragma once

#include <CLI/CLI.hpp>

class OptGroup {
  public:
    virtual ~OptGroup() = default;
    virtual std::string GroupName() = 0;
    virtual void RegisterOptions(CLI::App &app) = 0;
};

class OptGroupFieldmap final : public OptGroup {
  public:
    std::string base_game_preset_;
    std::size_t tiles_primary_override_;
    std::size_t tiles_total_override_;
    std::size_t metatiles_primary_override_;
    std::size_t metatiles_total_override_;
    std::size_t pals_primary_override_;
    std::size_t pals_total_override_;

    std::string GroupName() override {
        return "FIELDMAP OPTIONS";
    }

    void RegisterOptions(CLI::App &app) override {
        app.add_option("--base-game-preset", base_game_preset_, "Base game preset to use for the tileset.")
            ->group(GroupName());

        app.add_option("--tiles-primary-override", tiles_primary_override_,
                       "Override the number of tiles in the primary tileset.")
            ->check(CLI::Range(0UL, SIZE_MAX))
            ->group(GroupName());
        app.add_option("--tiles-total-override", tiles_total_override_,
                       "Override the total number of tiles in the tileset.")
            ->check(CLI::Range(0UL, SIZE_MAX))
            ->group(GroupName());

        app.add_option("--metatiles-primary-override", metatiles_primary_override_,
                       "Override the number of metatiles in the primary tileset.")
            ->check(CLI::Range(0UL, SIZE_MAX))
            ->group(GroupName());
        app.add_option("--metatiles-total-override", metatiles_total_override_,
                       "Override the total number of metatiles in the tileset.")
            ->check(CLI::Range(0UL, SIZE_MAX))
            ->group(GroupName());

        app.add_option("--pals-primary-override", pals_primary_override_,
                       "Override the number of metatiles in the primary tileset.")
            ->check(CLI::Range(0UL, SIZE_MAX))
            ->group(GroupName());
        app.add_option("--pals-total-override", pals_total_override_,
                       "Override the total number of metatiles in the tileset.")
            ->check(CLI::Range(0UL, SIZE_MAX))
            ->group(GroupName());
    }
};

class OptGroupDiagnostics final : public OptGroup {
  public:
    std::string GroupName() override {
        return "DIAGNOSTIC OPTIONS";
    }

    void RegisterOptions(CLI::App &app) override {}
};
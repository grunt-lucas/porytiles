#pragma once

#include <string>

#include "CLI/CLI.hpp"

#include "option.hpp"

// TODO: delete this dead code
//
// Nothing in Porytiles2 includes this header. OptGroupFieldmap is never instantiated, so its flags are never
// registered. The functionality this would have provided is already covered by the layered config system
// (e.g. TilesPalMode and LazyLayeredConfig).

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
        : base_game_preset_{"pokeemerald"}, tiles_primary_override_{0}, tiles_total_override_{0},
          metatiles_primary_override_{0}, metatiles_total_override_{0}, pals_primary_override_{0},
          pals_total_override_{0}
    {
    }

    [[nodiscard]] std::string GroupName() override
    {
        return "FIELDMAP OVERRIDE OPTIONS";
    }

    void RegisterGroup(CLI::App &app) override
    {
        app.add_option("--base-game-preset", base_game_preset_, "Base game preset to use for the tileset.")
            ->group(GroupName())
            ->capture_default_str();
        app.add_option(
               "--tiles-primary-override",
               tiles_primary_override_,
               "Override the number of tiles in the primary tileset.")
            ->group(GroupName());
        app.add_option(
               "--tiles-total-override", tiles_total_override_, "Override the total number of tiles in the tileset.")
            ->group(GroupName());
        app.add_option(
               "--metatiles-primary-override",
               metatiles_primary_override_,
               "Override the number of metatiles in the primary tileset.")
            ->group(GroupName());
        app.add_option(
               "--metatiles-total-override",
               metatiles_total_override_,
               "Override the total number of metatiles in the tileset.")
            ->group(GroupName());
        app.add_option(
               "--pals-primary-override",
               pals_primary_override_,
               "Override the number of metatiles in the primary tileset.")
            ->group(GroupName());
        app.add_option(
               "--pals-total-override", pals_total_override_, "Override the total number of metatiles in the tileset.")
            ->group(GroupName());
    }

    [[nodiscard]] const std::string &base_game_preset() const
    {
        return base_game_preset_;
    }
};

#pragma once

#include <CLI/CLI.hpp>

#include <porytiles/tiles/tiles_pal_mode.hpp>

#include "./validators.hpp"

class Opt {
  public:
    virtual ~Opt() = default;

    [[nodiscard]] virtual std::string NameShort() const = 0;

    [[nodiscard]] virtual std::string NameLong() const = 0;

    virtual void RegisterOpt(CLI::App &app) = 0;

    void SetGroup(const std::string &group, CLI::App &app) const {
        app.get_option(NameLong())->group(group);
    }

    [[nodiscard]] std::string NameCombined() const {
        return NameShort() + "," + NameLong();
    }
};

class OptOutput final : public Opt {
    std::string output_path_;

  public:
    OptOutput() : output_path_{"."} {}

    [[nodiscard]] std::string NameShort() const override {
        return "-o";
    }

    [[nodiscard]] std::string NameLong() const override {
        return "--output";
    }

    void RegisterOpt(CLI::App &app) override {
        app.add_option(NameCombined(), output_path_,
                       "Output generated files to the directory specified by PATH. If any element of PATH does not "
                       "exist, it will be created.")
            ->check(NotAlreadyAFileValidator{"PATH"})
            ->capture_default_str();
    }

    [[nodiscard]] std::string output_path() const {
        return output_path_;
    }
};

class OptTilesPalMode final : public Opt {
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
    OptTilesPalMode() : pal_format_{TilesPalModeToStr(porytiles::TilesPalMode::kTrueColor)} {}

    [[nodiscard]] std::string NameShort() const override {
        return "";
    }

    [[nodiscard]] std::string NameLong() const override {
        return "--tiles-pal-mode";
    }

    void RegisterOpt(CLI::App &app) override {
        app.add_option(
               NameLong(), pal_format_,
               "Set the palette mode for the output 'tiles.png'. Valid settings are 'true-color' or 'greyscale'. These "
               "settings are for human visual purposes only and have no effect on the final in-game tiles.")
            ->check(TilesPalModeValidator{"MODE"})
            ->capture_default_str();
    }

    [[nodiscard]] std::string pal_format() const {
        return pal_format_;
    }
};

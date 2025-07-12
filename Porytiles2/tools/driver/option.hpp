#pragma once

#include "CLI/CLI.hpp"

#include "porytiles2/infra/config/tiles_pal_mode.hpp"

#include "validators.hpp"

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
                       "Output generated files to the directory specified by PATH. "
                       "If any element of PATH does not "
                       "exist, it will be created.")
            ->check(NotAlreadyAFileValidator{})
            ->capture_default_str();
    }

    [[nodiscard]] std::string output_path() const {
        return output_path_;
    }
};

class OptTilesPalMode final : public Opt {
    std::string pal_format_;

  public:
    OptTilesPalMode() : pal_format_{tiles_pal_mode_to_str(porytiles2::TilesPalMode::true_color)} {}

    [[nodiscard]] std::string NameShort() const override {
        return "";
    }

    [[nodiscard]] std::string NameLong() const override {
        return "--tiles-pal-mode";
    }

    void RegisterOpt(CLI::App &app) override {
        app.add_option(NameLong(), pal_format_,
                       "Set the palette mode for the output 'tiles.png'. Valid "
                       "settings are 'true-color' or 'greyscale'. These "
                       "settings are for human visual purposes only and have no "
                       "effect on the final in-game tiles.")
            ->check(TilesPalModeValidator{})
            ->capture_default_str();
    }

    [[nodiscard]] std::string pal_format() const {
        return pal_format_;
    }
};

class OptDisableMetatileGeneration final : public Opt {
    bool disabled_{false};

  public:
    OptDisableMetatileGeneration() = default;

    [[nodiscard]] std::string NameShort() const override {
        return "";
    }

    [[nodiscard]] std::string NameLong() const override {
        return "--disable-metatile-generation";
    }

    void RegisterOpt(CLI::App &app) override {
        app.add_flag(NameLong(), disabled_,
                     "Disable generation of 'metatiles.bin'. Only enable this if "
                     "you want to manage metatiles manually "
                     "via Porymap.");
    }

    [[nodiscard]] bool disabled() const {
        return disabled_;
    }
};

class OptDisableAttributeGeneration final : public Opt {
    bool disabled_{false};

  public:
    OptDisableAttributeGeneration() = default;

    [[nodiscard]] std::string NameShort() const override {
        return "";
    }

    [[nodiscard]] std::string NameLong() const override {
        return "--disable-attribute-generation";
    }

    void RegisterOpt(CLI::App &app) override {
        app.add_flag(NameLong(), disabled_,
                     "Disable generation of 'metatile_attributes.bin'. Only enable "
                     "this if you want to manage metatile "
                     "attributes manually via Porymap.");
    }

    [[nodiscard]] bool disabled() const {
        return disabled_;
    }
};

class OptTripleLayer final : public Opt {
    bool triple_layer_{false};

  public:
    OptTripleLayer() = default;

    [[nodiscard]] std::string NameShort() const override {
        return "";
    }

    [[nodiscard]] std::string NameLong() const override {
        return "--triple-layer";
    }

    void RegisterOpt(CLI::App &app) override {
        app.add_flag(NameLong(), triple_layer_,
                     "Enable triple-layer compilation mode. If this option is not "
                     "supplied, Porytiles assumes you are compiling "
                     "a dual-layer tileset. For dual-layer tilesets the layer type "
                     "will be inferred from your source layer "
                     "PNGs, so compilation will error out if any metatiles contain "
                     "content on all three layers.");
    }

    [[nodiscard]] bool dual_layer() const {
        return triple_layer_;
    }
};

class OptTransparencyColor final : public Opt {
    std::string rgb_;

  public:
    OptTransparencyColor() : rgb_{"255,0,255"} {}

    [[nodiscard]] std::string NameShort() const override {
        return "";
    }

    [[nodiscard]] std::string NameLong() const override {
        return "--transparency-color";
    }

    void RegisterOpt(CLI::App &app) override {
        app.add_option(NameLong(), rgb_,
                       "Select RGB color <R,G,B> to represent transparency in your "
                       "layer source PNGs.")
            ->check(RgbStringValidator{})
            ->capture_default_str();
    }

    [[nodiscard]] const std::string &rgb() const {
        return rgb_;
    }
};

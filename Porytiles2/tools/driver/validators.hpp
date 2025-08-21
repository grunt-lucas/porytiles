#pragma once

#include <expected>
#include <filesystem>
#include <unordered_set>

#include "CLI/CLI.hpp"
#include "fmt/format.h"

#include "porytiles2/infra/config/tiles_pal_mode.hpp"
#include "porytiles2/infra/diagnostics/diagnostics.hpp"
#include "porytiles2/infra/utilities/utilities.hpp"
#include "porytiles2/templates/parsing.hpp"

class TilesPalModeValidator final : public CLI::Validator {
    static constexpr auto kHint = "MODE";

  public:
    explicit TilesPalModeValidator() : Validator{kHint}
    {
        name_ = "TILES_OUTPUT_PAL";
        func_ = [](const std::string &str) {
            if (!porytiles2::tiles_pal_mode_from_str(str).has_value()) {
                return std::string{"invalid 'tiles.png' output palette mode: " + str};
            }
            return std::string{};
        };
    }
};

class NotAlreadyAFileValidator final : public CLI::Validator {
    static constexpr auto kHint = "PATH";

  public:
    explicit NotAlreadyAFileValidator() : Validator{kHint}
    {
        name_ = "NOT_ALREADY_A_FILE";
        func_ = [](const std::string &str) {
            if (std::filesystem::exists(str) && std::filesystem::is_regular_file(str)) {
                return std::string{"file already exists: " + str};
            }
            return std::string{};
        };
    }
};

class RgbStringValidator final : public CLI::Validator {
    static constexpr auto kHint = "R,G,B";

  public:
    explicit RgbStringValidator() : Validator{kHint}
    {
        name_ = "RGB_STRING";
        func_ = [](const std::string &str) {
            const std::vector<std::string> color_components = porytiles2::split(str, ",");
            if (color_components.size() != 3) {
                return std::string{"invalid rgb string: " + str};
            }

            const auto red_result = porytiles2::parse_int<int>(color_components[0]);
            const auto green_result = porytiles2::parse_int<int>(color_components[1]);
            const auto blue_result = porytiles2::parse_int<int>(color_components[2]);

            if (!red_result.has_value()) {
                return std::string{"invalid rgb red component: " + red_result.error()};
            }
            if (!green_result.has_value()) {
                return std::string{"invalid rgb green component: " + green_result.error()};
            }
            if (!blue_result.has_value()) {
                return std::string{"invalid rgb blue component: " + blue_result.error()};
            }

            const auto red = red_result.value();
            const auto green = green_result.value();
            const auto blue = blue_result.value();

            if (red < 0 || red > 255) {
                return fmt::format("rgb red component out of range: {}", red);
            }
            if (green < 0 || green > 255) {
                return fmt::format("rgb green component out of range: {}", green);
            }
            if (blue < 0 || blue > 255) {
                return fmt::format("rgb blue component out of range: {}", blue);
            }

            return std::string{};
        };
    }
};

class DiagnosticIsWarningValidator final : public CLI::Validator {
    static constexpr auto kHint = "DIAG";

  public:
    explicit DiagnosticIsWarningValidator() : Validator{kHint}
    {
        name_ = "DIAGNOSTIC_IS_WARNING";
        std::unordered_set<std::string> warning_diags;
        for (const auto name : porytiles2::all_diag_names(porytiles2::DiagLevel::warning)) {
            warning_diags.insert(name);
        }
        func_ = [warning_diags](const std::string &str) {
            if (warning_diags.contains(str)) {
                return std::string{};
            }
            return std::string{"invalid warning diagnostic: " + str};
        };
    }
};

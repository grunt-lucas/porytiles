#pragma once

#include <filesystem>

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <porytiles/legacy/utilities.h>
#include <porytiles/template_lib/parsing.hpp>
#include <porytiles/template_lib/result.hpp>

class TilesPalModeValidator final : public CLI::Validator {
    static constexpr auto kHint = "MODE";

  public:
    explicit TilesPalModeValidator() : Validator{kHint} {
        name_ = "TILES_OUTPUT_PAL";
        func_ = [](const std::string &str) {
            if (!porytiles::TilesPalModeFromStr(str).has_value()) {
                return std::string{"invalid 'tiles.png' output palette mode: " + str};
            }
            return std::string{};
        };
    }
};

class NotAlreadyAFileValidator final : public CLI::Validator {
    static constexpr auto kHint = "PATH";

  public:
    explicit NotAlreadyAFileValidator() : Validator{kHint} {
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
    explicit RgbStringValidator() : Validator{kHint} {
        name_ = "RGB_STRING";
        func_ = [](const std::string &str) {
            const std::vector<std::string> colorComponents = porytiles::split(str, ",");
            if (colorComponents.size() != 3) {
                return std::string{"invalid rgb string: " + str};
            }

            const auto redResult = porytiles::ParseInt<int>(colorComponents[0]);
            const auto greenResult = porytiles::ParseInt<int>(colorComponents[1]);
            const auto blueResult = porytiles::ParseInt<int>(colorComponents[2]);

            if (!redResult.HasSuccess()) {
                return std::string{"invalid rgb red component: " + colorComponents[0]};
            }
            if (!greenResult.HasSuccess()) {
                return std::string{"invalid rgb green component: " + colorComponents[1]};
            }
            if (!blueResult.HasSuccess()) {
                return std::string{"invalid rgb blue component: " + colorComponents[2]};
            }

            const auto red = redResult.Get();
            const auto green = greenResult.Get();
            const auto blue = blueResult.Get();

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
    explicit DiagnosticIsWarningValidator() : Validator{kHint} {
        name_ = "DIAGNOSTIC_IS_WARNING";
        std::unordered_set<std::string> warning_diags;
        for (const auto name : porytiles::AllDiagNames(porytiles::DiagLevel::Warning)) {
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

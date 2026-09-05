#pragma once

#include <expected>
#include <filesystem>
#include <format>
#include <unordered_set>

#include "CLI/CLI.hpp"

#include "porytiles/domain/algorithms/color_search.hpp"
#include "porytiles/domain/config/tiles_palette_mode.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/parse_int.hpp"
#include "porytiles/utilities/string_utils.hpp"

class NotAlreadyAFileValidator final : public CLI::Validator {
    static constexpr auto hint = "PATH";

  public:
    explicit NotAlreadyAFileValidator() : Validator{hint}
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

/// @brief Validates a color argument in the "R,G,B" or "R,G,B,A" syntax that porytiles::parse_rgba32_string accepts.
class Rgba32ColorValidator final : public CLI::Validator {
    static constexpr auto hint = "R,G,B";

  public:
    explicit Rgba32ColorValidator() : Validator{hint}
    {
        name_ = "RGB_COLOR";
        func_ = [](const std::string &str) {
            const auto parsed = porytiles::parse_rgba32_string(str);
            if (!parsed.has_value()) {
                return std::string{"invalid color '" + str + "': " + parsed.error()};
            }
            return std::string{};
        };
    }
};

/// @brief Validates a color tolerance in the syntax porytiles::parse_color_tolerance accepts: an integer in 0-255, or
/// the word "gba" for GBA 15-bit color equivalence.
class ColorToleranceValidator final : public CLI::Validator {
    static constexpr auto hint = "N|gba";

  public:
    explicit ColorToleranceValidator() : Validator{hint}
    {
        name_ = "COLOR_TOLERANCE";
        func_ = [](const std::string &str) {
            const auto parsed = porytiles::parse_color_tolerance(str);
            if (!parsed.has_value()) {
                return std::string{"invalid tolerance: " + parsed.error()};
            }
            return std::string{};
        };
    }
};

/// @brief Validates a match limit: a positive integer, or the word "all" for no cap.
class MatchLimitValidator final : public CLI::Validator {
    static constexpr auto hint = "N|all";

  public:
    explicit MatchLimitValidator() : Validator{hint}
    {
        name_ = "MATCH_LIMIT";
        func_ = [](const std::string &str) {
            if (str == "all") {
                return std::string{};
            }
            const auto parsed = porytiles::parse_int<std::size_t>(str, 10);
            if (!parsed.has_value() || parsed.value() == 0) {
                return std::string{"invalid limit '" + str + "': must be a positive integer or 'all'"};
            }
            return std::string{};
        };
    }
};

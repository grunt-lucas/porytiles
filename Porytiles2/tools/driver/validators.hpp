#pragma once

#include <expected>
#include <filesystem>
#include <format>
#include <unordered_set>

#include "CLI/CLI.hpp"

#include "porytiles2/domain/config/tiles_pal_mode.hpp"
#include "porytiles2/utilities/parse_int.hpp"
#include "porytiles2/utilities/string_utils.hpp"

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

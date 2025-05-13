#pragma once

#include <filesystem>

#include <CLI/CLI.hpp>

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
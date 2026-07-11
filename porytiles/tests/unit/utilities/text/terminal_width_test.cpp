#include "gtest/gtest.h"

#include <cstdlib>
#include <string>

#include "porytiles/utilities/text/terminal_width.hpp"

using namespace porytiles;

namespace {

/// @brief Restores (or clears) the COLUMNS environment variable when the test scope exits.
class ColumnsEnvGuard {
  public:
    ColumnsEnvGuard()
    {
        const char *existing = std::getenv("COLUMNS");
        if (existing != nullptr) {
            had_value_ = true;
            saved_ = existing;
        }
    }

    ColumnsEnvGuard(const ColumnsEnvGuard &) = delete;
    ColumnsEnvGuard &operator=(const ColumnsEnvGuard &) = delete;
    ColumnsEnvGuard(ColumnsEnvGuard &&) = delete;
    ColumnsEnvGuard &operator=(ColumnsEnvGuard &&) = delete;

    ~ColumnsEnvGuard()
    {
        if (had_value_) {
            setenv("COLUMNS", saved_.c_str(), 1);
        }
        else {
            unsetenv("COLUMNS");
        }
    }

  private:
    bool had_value_ = false;
    std::string saved_;
};

} // namespace

TEST(TerminalWidthTests, ColumnsEnvOverridesEverything)
{
    ColumnsEnvGuard guard;
    setenv("COLUMNS", "123", 1);
    // fd -1 is not a terminal, so only the env var can supply the width.
    EXPECT_EQ(resolve_terminal_width(-1, 80), 123U);
}

TEST(TerminalWidthTests, FallbackWhenNoTerminalAndNoEnv)
{
    ColumnsEnvGuard guard;
    unsetenv("COLUMNS");
    EXPECT_EQ(resolve_terminal_width(-1, 80), 80U);
}

TEST(TerminalWidthTests, InvalidColumnsFallsBack)
{
    ColumnsEnvGuard guard;
    setenv("COLUMNS", "not-a-number", 1);
    EXPECT_EQ(resolve_terminal_width(-1, 100), 100U);
}

TEST(TerminalWidthTests, NonPositiveColumnsFallsBack)
{
    ColumnsEnvGuard guard;
    setenv("COLUMNS", "0", 1);
    EXPECT_EQ(resolve_terminal_width(-1, 77), 77U);
}

TEST(TerminalWidthTests, TrailingJunkColumnsFallsBack)
{
    ColumnsEnvGuard guard;
    setenv("COLUMNS", "80x", 1);
    EXPECT_EQ(resolve_terminal_width(-1, 42), 42U);
}

#pragma once

#include <CLI/CLI.hpp>
#include <string>

/// Collection of all options for 'compile-secondary'.
struct CompileSecondaryOpts {
    std::string file;
    bool with_foo;
};

void SetupCompileSecondary(CLI::App &app);
void RunCompileSecondary(const CompileSecondaryOpts &opt);

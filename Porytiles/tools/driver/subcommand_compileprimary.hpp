#pragma once

#include <CLI/CLI.hpp>
#include <string>

/// Collection of all options for 'compile-primary'.
struct CompilePrimaryOpts {
    std::string file;
    bool with_foo;
    bool with_bar;
};

void SetupCompilePrimary(CLI::App &app);
void RunCompilePrimary(const CompilePrimaryOpts &opt);

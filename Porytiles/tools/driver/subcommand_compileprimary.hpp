#pragma once

#include <CLI/CLI.hpp>
#include <string>

#include "./optiongroup.hpp"

/// Collection of all options for 'compile-primary'.
struct CompilePrimaryOpts {
    OptGroupFieldmap fieldmapOpts;
};

void SetupCompilePrimary(CLI::App &app);
void RunCompilePrimary(const CompilePrimaryOpts &opt);

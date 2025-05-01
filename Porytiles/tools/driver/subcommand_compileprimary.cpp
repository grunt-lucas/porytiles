#include "./subcommand_compileprimary.hpp"

/// Set up a subcommand and capture a shared_ptr to a struct that holds all its options.
/// The variables of the struct are bound to the CLI options.
/// We use a shared ptr so that the addresses of the variables remain for binding,
/// You could return the shared pointer if you wanted to access the values in main.
void SetupCompilePrimary(CLI::App &app) {
    // Create the option and subcommand objects.
    auto opt = std::make_shared<CompilePrimaryOpts>();
    auto *sub = app.add_subcommand("compile-primary", "Compile a primary tileset using explicit asset paths");

    // Add options to sub, binding them to opt.
    sub->add_flag("--with-foo", opt->with_foo, "Counter");

    // Set the run function as callback to be called when this subcommand is issued.
    sub->callback([opt]() { RunCompilePrimary(*opt); });
}

/// The function that runs our code.
/// This could also simply be in the callback lambda itself,
/// but having a separate function is cleaner.
void RunCompilePrimary(const CompilePrimaryOpts &opt) {
    std::cout << "Working on file: " << opt.file << '\n';
    if (opt.with_foo) {
        std::cout << "Using foo!" << '\n';
    }
}
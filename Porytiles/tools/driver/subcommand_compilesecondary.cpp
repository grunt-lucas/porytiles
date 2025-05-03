#include "./subcommand_compilesecondary.hpp"

void SetupCompileSecondary(CLI::App &app) {
    auto opt = std::make_shared<CompileSecondaryOpts>();
    auto *sub = app.add_subcommand("compile-secondary", "Compile a secondary tileset using explicit asset paths");
    sub->group("LEGACY SUBCOMMANDS");

    sub->add_flag("--with-foo", opt->with_foo, "Counter");
    sub->callback([opt]() { RunCompileSecondary(*opt); });
}

void RunCompileSecondary(const CompileSecondaryOpts &opt) {
    std::cout << "Working on file: " << opt.file << '\n';
    if (opt.with_foo) {
        std::cout << "Using foo!" << '\n';
    }
}
#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <porytiles/build_version.h>

#include "./command.hpp"

int main(const int argc, char **argv) {
    CLI::App app{"Porytiles"};

    app.description(fmt::format(R"(porytiles {} {}
grunt-lucas <grunt.lucas@yahoo.com>

Overworld tileset compiler for use with the pokeruby, pokefirered, and
pokeemerald Pokémon Generation III decompilation projects from pret. Also
compatible with pokeemerald-expansion from rh-hideout. Builds Porymap-ready
tilesets from RGBA (or indexed) tile assets.

Home Page: https://github.com/grunt-lucas/porytiles)",
                                std::string{PORYTILES_BUILD_VERSION}, std::string{PORYTILES_BUILD_DATE}));

    app.footer(
        R"(To get more help with Porytiles, check out the guides at:
https://github.com/grunt-lucas/porytiles/wiki
https://www.youtube.com/playlist?list=PLuyjFojPxF7-O5o_mS6uTBtyYcuyFf_Ce

SEE ALSO
https://github.com/pret/pokeruby
https://github.com/pret/pokefirered
https://github.com/pret/pokeemerald
https://github.com/rh-hideout/pokeemerald-expansion
https://github.com/huderlem/porymap)");

    app.add_flag("-V,--version", [](const size_t) {
        std::cout << PORYTILES_EXECUTABLE << " " << PORYTILES_BUILD_VERSION << " " << PORYTILES_BUILD_DATE << std::endl;
        std::exit(0);
    });

    CompilePrimaryCommand compilePrimaryCommand{};
    compilePrimaryCommand.Setup(app);

    // Make sure we get at least one subcommand
    app.require_subcommand();

    // More setup if needed, i.e., other subcommands etc.

    CLI11_PARSE(app, argc, argv);
    return 0;
}

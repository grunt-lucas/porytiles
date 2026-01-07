#include <format>

#include "CLI/CLI.hpp"

#include "porytiles2/build_version.h"

#include "command_compile_tileset.hpp"
#include "command_defunct_import_tileset.hpp"

int main(const int argc, char **argv)
{
    CLI::App porytiles_app{"Porytiles"};

    porytiles_app.description(
        std::format(
            R"(porytiles {} {}
grunt-lucas <grunt.lucas@yahoo.com>

Overworld tileset compiler for use with the pokeruby, pokefirered, and
pokeemerald Pokémon Generation III decompilation projects from pret. Also
compatible with pokeemerald-expansion from rh-hideout. Builds Porymap-ready
assets from RGBA (or indexed) input assets.

Home Page: https://github.com/grunt-lucas/porytiles)",
            std::string{PORYTILES_BUILD_VERSION},
            std::string{PORYTILES_BUILD_DATE}));

    porytiles_app.footer(
        R"(To get more help with Porytiles, check out the guides at:
https://github.com/grunt-lucas/porytiles/wiki
https://www.youtube.com/playlist?list=PLuyjFojPxF7-O5o_mS6uTBtyYcuyFf_Ce

SEE ALSO
https://github.com/pret/pokeruby
https://github.com/pret/pokefirered
https://github.com/pret/pokeemerald
https://github.com/rh-hideout/pokeemerald-expansion
https://github.com/huderlem/porymap)");

    // Override some --version,--help flag defaults.
    porytiles_app.add_flag(
        "-V,--version",
        [](const size_t) {
            std::cout << PORYTILES_EXECUTABLE << " " << PORYTILES_BUILD_VERSION << " " << PORYTILES_BUILD_DATE
                      << std::endl;
            std::exit(0);
        },
        "Print version info and exit.");
    porytiles_app.get_option("--help")->description("Print this help message and exit.");

    DefunctImportTilesetCommand defunct_import_tileset{porytiles_app};
    CompileTilesetCommand compile_tileset{porytiles_app};

    porytiles_app.require_subcommand();

    CLI11_PARSE(porytiles_app, argc, argv);
    return 0;
}

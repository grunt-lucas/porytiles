#include "CLI/CLI.hpp"
#include "fmt/format.h"

#include "porytiles2/build_version.h"

#include "command.hpp"
#include "create_tileset_command.hpp"
#include "debug_commands.hpp"
#include "verify_tileset_command.hpp"

int main(const int argc, char **argv)
{
    CLI::App porytiles_app{"Porytiles"};

    porytiles_app.description(
        fmt::format(
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

    CreateTilesetCommand create_tileset{porytiles_app};
    VerifyTilesetCommand verify_tileset{porytiles_app};
    DebugNormalizeCommand debug{porytiles_app};

    porytiles_app.require_subcommand();

    CLI11_PARSE(porytiles_app, argc, argv);
    return 0;
}

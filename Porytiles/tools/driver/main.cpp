#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <porytiles/build_version.h>

#include "./command.hpp"

int main(const int argc, char **argv) {
    CLI::App porytiles_app{"Porytiles"};

    porytiles_app.description(fmt::format(R"(porytiles {} {}
grunt-lucas <grunt.lucas@yahoo.com>

Overworld tileset compiler for use with the pokeruby, pokefirered, and
pokeemerald Pokémon Generation III decompilation projects from pret. Also
compatible with pokeemerald-expansion from rh-hideout. Builds Porymap-ready
assets from RGBA (or indexed) input assets.

Home Page: https://github.com/grunt-lucas/porytiles)",
                                          std::string{PORYTILES_BUILD_VERSION}, std::string{PORYTILES_BUILD_DATE}));

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

    porytiles_app.add_flag(
        "-V,--version",
        [](const size_t) {
            std::cout << PORYTILES_EXECUTABLE << " " << PORYTILES_BUILD_VERSION << " " << PORYTILES_BUILD_DATE
                      << std::endl;
            std::exit(0);
        },
        "Print version info and exit");

    CompilePrimaryCommand compile_primary{porytiles_app};
    CompileSecondaryCommand compile_secondary{porytiles_app};
    DecompilePrimaryCommand decompile_primary{porytiles_app};
    DecompileSecondaryCommand decompile_secondary{porytiles_app};

    CompileTilesetCommand compile_tileset{porytiles_app};
    CompileLayoutCommand compile_layout{porytiles_app};
    CompileSpritesheetCommand compile_spritesheet{porytiles_app};

    DecompileTilesetCommand decompile_tileset{porytiles_app};
    DecompileLayoutCommand decompile_layout{porytiles_app};

    ReduceBitDepthCommand reduce_bit_depth{porytiles_app};

    porytiles_app.require_subcommand();

    CLI11_PARSE(porytiles_app, argc, argv);
    return 0;
}

#include <format>
#include <memory>

#include "CLI/CLI.hpp"

#include "porytiles/build_version.h"

#include "command_compile_tileset.hpp"
#include "command_completion.hpp"
#include "command_create_tileset.hpp"
#include "command_decompile_tileset.hpp"
#include "command_dump_attribute_schema.hpp"
#include "command_dump_project_config.hpp"
#include "command_dump_tileset_colors.hpp"
#include "command_dump_tileset_config.hpp"
#include "command_edit_project_config.hpp"
#include "command_edit_tileset_config.hpp"
#include "command_find_tileset_color.hpp"
#include "command_import_tileset.hpp"
#include "command_list_tilesets.hpp"
#include "custom_formatter.hpp"

int main(const int argc, char **argv)
{
    CLI::App porytiles_app{"Porytiles"};

    // Set custom formatter for cleaner help output (inherited by subcommands)
    porytiles_app.formatter(std::make_shared<porytiles::PorytilesFormatter>());

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
        R"(To get more help with Porytiles, check out the following guides:

USER DOCUMENTATION
https://grunt-lucas.github.io/porytiles-user-docs

DEVELOPER DOCUMENTATION
https://grunt-lucas.github.io/porytiles-dev-docs

DOXYGEN SOURCE CODE DOCUMENTATION
https://grunt-lucas.github.io/porytiles

COMMUNITY-MADE TUTORIAL VIDEOS
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
    ImportTilesetCommand import_tileset{porytiles_app};
    CompileTilesetCommand compile_tileset{porytiles_app};
    DecompileTilesetCommand decompile_tileset{porytiles_app};
    DumpTilesetConfigCommand dump_tileset_config{porytiles_app};
    DumpProjectConfigCommand dump_project_config{porytiles_app};
    EditTilesetConfigCommand edit_tileset_config{porytiles_app};
    EditProjectConfigCommand edit_project_config{porytiles_app};
    DumpAttributeSchemaCommand dump_attribute_schema{porytiles_app};
    FindTilesetColorCommand find_tileset_color{porytiles_app};
    DumpTilesetColorsCommand dump_tileset_colors{porytiles_app};
    CompletionCommand completion{porytiles_app};
    ListTilesetsCommand list_tilesets{porytiles_app};

    porytiles_app.require_subcommand();

    CLI11_PARSE(porytiles_app, argc, argv);
    return 0;
}

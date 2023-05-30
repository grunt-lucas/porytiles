#include "tscreate.h"

#include <iostream>

#include <cxxopts.hpp>

bool parse(int argc, const char* argv[]) {
    try {
        std::unique_ptr<cxxopts::Options> allocated(
            new cxxopts::Options(argv[0], "Create a pokeemerald indexed tileset from a master PNG")
        );
        auto& options = *allocated;

        options.add_options()
            ("master", "The path to the master PNG file", cxxopts::value<std::string>())
            ("output", "The desired output path", cxxopts::value<std::string>())
            ("h,help", "Print usage");
        options.parse_positional({"master", "output"});
        options.positional_help("</path/to/master.png> </path/to/output/dir>");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(0);
        }
    }
    catch (const cxxopts::exceptions::exception& e) {
        std::cout << argv[0] << ": optarg error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

int main(int argc, const char* argv[]) {
    if (!parse(argc, argv)) {
        return 1;
    }
    return 0;
}

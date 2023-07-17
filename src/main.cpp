#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <iostream>

#include "config.h"
#include "cli_parser.h"

int main(int argc, char** argv) {
    porytiles::Config config = porytiles::defaultConfig();
    porytiles::parseOptions(config, argc, argv);

    return 0;
}

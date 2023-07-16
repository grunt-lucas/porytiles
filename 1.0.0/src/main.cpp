#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <iostream>

#include "cli_parser.h"

int main(int argc, char** argv) {
    porytiles::parseOptions(argc, argv);

    return 0;
}

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

int returnFive() {
    return 5;
}

TEST_CASE("It should return 5") {
    CHECK(returnFive() == 5);
}

TEST_CASE("It should not return 6") {
    CHECK(returnFive() != 6);
}

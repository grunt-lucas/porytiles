#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest.h>

#include <iostream>

int main(int argc, char** argv) {
#if defined(__GNUG__) && !defined(__clang__)
    std::cout << "built with g++" << std::endl;
#else
    std::cout << "built with clang (or something else)" << std::endl;
#endif
    return 0;
}

#include <iostream>
#include <string>

#include "porytiles/Color/Bgr15.h"

int main()
{
    using namespace porytiles::color;
    const Bgr15 bgr{};
    std::cout << "Hello, Porytiles!" << std::endl;
    std::cout << "My BGR raw value: " << std::to_string(bgr.getRawValue()) << std::endl;
}

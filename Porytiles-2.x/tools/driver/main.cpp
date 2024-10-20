#include <iostream>
#include <string>

#include "porytiles/Color/Bgr15.h"
#include "porytiles/Color/Rgba32.h"

int main()
{
    using namespace porytiles::color;
    const Bgr15 bgr{};
    const Rgba32 rgb{};
    std::cout << "Hello, Porytiles!" << std::endl;
    std::cout << "My BGR raw value: " << std::to_string(bgr.getRawValue()) << std::endl;
    std::cout << "My RGB jasc string: " << rgb.toJascString() << std::endl;
}

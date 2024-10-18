#ifndef PORYTILES_COLOR_BGR15_H
#define PORYTILES_COLOR_BGR15_H

#include <cstdint>

namespace porytiles {
namespace color {

class Bgr15 {
    std::uint16_t bgr;

public:
    Bgr15() : bgr{0} {}
    std::uint16_t getBgr() const;
};

} // end namespace color
} // end namespace porytiles

#endif // PORYTILES_COLOR_BGR15_H

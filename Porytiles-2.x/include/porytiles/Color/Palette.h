#ifndef PORYTILES_COLOR_PALETTE_H
#define PORYTILES_COLOR_PALETTE_H

#include <vector>

#include <porytiles/Color/ColorConverter.h>
#include <porytiles/Color/RGBLike.h>

namespace porytiles::color {

template<RGBLike T>
class Palette {
    std::vector<T> colors;

  public:
    explicit Palette(std::size_t capacity) {
        colors.reserve(capacity);
    }

    [[nodiscard]] std::size_t getCapacity() const {
        return colors.capacity();
    }

    [[nodiscard]] std::size_t getSize() const {
        return colors.size();
    }

    const T& at(std::size_t index) const {
        return colors.at(index);
    }

    void pushColor(const T& color) {
        if (getSize() >= getCapacity()) {
            throw std::out_of_range("porytiles::color::Palette::pushColor");
        }
        colors.push_back(color);
    }

    template<RGBLike U>
    void foo(const ColorConverter<U, T>& converter, U color) {
        auto myColor = converter.convert(color);
        std::cout << ":" << myColor.toJascString() << std::endl;
    }
};

} // namespace porytiles::color

#endif // PORYTILES_COLOR_PALETTE_H
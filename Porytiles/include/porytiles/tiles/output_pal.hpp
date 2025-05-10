#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "../panic/panic.hpp"

namespace porytiles {

class OutputPalette {
  public:
    enum Value : std::uint8_t { kTrueColor, kGreyscale };

    static std::optional<OutputPalette> FromString(const std::string &str) {
        if (str == "true-color") {
            return OutputPalette{kTrueColor};
        }
        if (str == "greyscale") {
            return OutputPalette{kGreyscale};
        }
        return std::nullopt;
    }

    OutputPalette() = default;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    // NOLINTNEXTLINE
    constexpr OutputPalette(const Value v) : value(v) {}

    // Allow switch and comparisons.
    // constexpr operator Value() const { return value; }

    // Prevent usage: if(pal)
    explicit operator bool() const = delete;

    constexpr bool operator==(const OutputPalette &p) const {
        return value == p.value;
    }

    constexpr bool operator!=(const OutputPalette &p) const {
        return value != p.value;
    }

    [[nodiscard]] std::string ToString() const {
        switch (value) {
        case kTrueColor:
            return "true-color";
        case kGreyscale:
            return "greyscale";
        }
        Panic("unhandled OutputPalette value");
    }

  private:
    Value value;
};

} // namespace porytiles

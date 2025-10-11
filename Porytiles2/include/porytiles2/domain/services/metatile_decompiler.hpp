#pragma once

namespace porytiles2 {

/**
 * @brief Represents a foo.
 */
class MetatileDecompiler {
public:
    MetatileDecompiler() = default;
    ~MetatileDecompiler() = default;

    [[nodiscard]] int foo() const;

private:
    int foo_{};
};

} // namespace porytiles2

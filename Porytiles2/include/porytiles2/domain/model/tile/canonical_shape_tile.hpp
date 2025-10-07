#pragma once

namespace porytiles2 {

class CanonicalShapeTile {
  public:
    CanonicalShapeTile() = default;
    ~CanonicalShapeTile() = default;

    [[nodiscard]] int foo() const;

  private:
    int foo_{};
};

} // namespace porytiles2

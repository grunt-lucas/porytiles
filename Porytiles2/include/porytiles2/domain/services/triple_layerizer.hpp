#pragma once

namespace porytiles2 {

class TripleLayerizer {
  public:
    TripleLayerizer() = default;
    ~TripleLayerizer() = default;

    [[nodiscard]] int foo() const;

  private:
    int foo_{};
};

} // namespace porytiles2

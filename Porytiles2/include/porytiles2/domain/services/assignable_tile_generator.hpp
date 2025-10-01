#pragma once

#include "porytiles2/domain/model/assignable_tile.hpp"

namespace porytiles2 {

/**
 * @brief Represents a foo.
 */
class AssignableTileGenerator {
  public:
    AssignableTileGenerator() = default;
    ~AssignableTileGenerator() = default;

    [[nodiscard]] int foo() const;

    [[nodiscard]] AssignableTile generate() const;

  private:
    int foo_{};
};

} // namespace porytiles2

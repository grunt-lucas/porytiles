#pragma once

#include <memory>

#include "porytiles2/infra/diagnostics/DiagnosticEngine.hpp"

namespace porytiles2 {

/**
 * @brief A factory for creating and retrieving a singleton DiagEngine.
 *
 * @details
 * A factory for creating and retrieving a singleton DiagEngine.
 */
class DiagEngineFactory {
public:
  /**
   * @brief Gets the singleton DiagEngine instance.
   *
   * @details
   * The DiagEngine is lazily instantiated with a StderrConsumer the first time
   * this function is called. Subsequent calls will return the same instance.
   *
   * @return A reference to the singleton DiagEngine.
   */
  static DiagEngine &engine() {
    if (override_) {
      return *override_;
    }
    // C++11 guarantees that the initialization of static local variables is
    // thread-safe.
    static DiagEngine engine{std::make_unique<StderrConsumer>()};
    return engine;
  }

  static void override_engine(DiagEngine *engine) { override_ = engine; }

private:
  // A raw pointer is used here. The test fixture will own the mock object.
  inline static DiagEngine *override_ = nullptr;
};

} // namespace porytiles2

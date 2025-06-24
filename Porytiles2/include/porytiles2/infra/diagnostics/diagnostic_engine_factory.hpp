#pragma once

#include <memory>

#include <porytiles2/infra/diagnostics/diagnostic_engine.hpp>

namespace porytiles {

/**
 * @brief A factory for creating and retrieving a singleton DiagEngine.
 */
class DiagEngineFactory {
  public:
    /**
     * @brief Gets the singleton DiagEngine instance.
     *
     * @details
     * The DiagEngine is lazily instantiated with a StderrConsumer the first time this function is called.
     * Subsequent calls will return the same instance.
     *
     * @return A reference to the singleton DiagEngine.
     */
    static DiagEngine &GetEngine() {
        // C++11 guarantees that the initialization of static local variables is thread-safe.
        static DiagEngine engine{std::make_unique<StderrConsumer>()};
        return engine;
    }
};

} // namespace porytiles

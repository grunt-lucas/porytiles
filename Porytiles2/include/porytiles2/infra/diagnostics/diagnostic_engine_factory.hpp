#pragma once

#include <memory>

#include <porytiles2/infra/diagnostics/diagnostic_engine.hpp>

namespace porytiles {

/**
 * @brief A factory for creating and retrieving a singleton DiagnosticEngine.
 */
class DiagnosticEngineFactory {
  public:
    /**
     * @brief Gets the singleton DiagnosticEngine instance.
     *
     * @details
     * The DiagnosticEngine is lazily instantiated with a StderrConsumer the first time this function is called.
     * Subsequent calls will return the same instance.
     *
     * @return A reference to the singleton DiagnosticEngine.
     */
    static DiagEngine &GetEngine() {
        // C++11 guarantees that the initialization of static local variables is thread-safe.
        static DiagEngine engine{std::make_unique<StderrConsumer>()};
        return engine;
    }
};

} // namespace porytiles

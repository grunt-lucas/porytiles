# Domain Events + Event Handlers Implementation Plan for Porytiles2 Diagnostics

## Executive Summary

This document outlines a comprehensive implementation plan for integrating diagnostic capabilities into Porytiles2 using the Domain Events + Event Handlers pattern. This approach maintains domain purity while providing rich diagnostic information throughout the tileset compilation process.

## Architecture Overview

The implementation follows a three-layer architecture:

1. **Domain Layer**: Defines event interfaces and domain-specific events
2. **Infrastructure Layer**: Implements event bus, handlers, and diagnostic integration
3. **Application Layer**: Orchestrates event flow and handler registration

## Core Components

### 1. Domain Layer Components

#### 1.1 Base Event Infrastructure

```c++
// File: Porytiles2/include/porytiles2/domain/events/DomainEvent.hpp

namespace porytiles2 {

/**
 * @brief Base class for all domain events
 * 
 * @details
 * Provides common metadata for all events including timestamp and event ID
 */
class DomainEvent {
  public:
    using EventId = std::string;
    using Timestamp = std::chrono::system_clock::time_point;

    DomainEvent() : event_id_{generate_event_id()}, occurred_at_{std::chrono::system_clock::now()} {}
    virtual ~DomainEvent() = default;

    [[nodiscard]] const EventId &event_id() const { return event_id_; }
    [[nodiscard]] const Timestamp &occurred_at() const { return occurred_at_; }
    [[nodiscard]] virtual std::string event_name() const = 0;

  private:
    EventId event_id_;
    Timestamp occurred_at_;

    static EventId generate_event_id();
};

/**
 * @brief Interface for event bus that domain services use
 */
class DomainEventBus {
  public:
    virtual ~DomainEventBus() = default;
    
    /**
     * @brief Emit an event to the bus
     * 
     * @param event The event to emit
     */
    virtual void emit(std::unique_ptr<DomainEvent> event) = 0;
};

/**
 * @brief Null implementation for testing
 */
class NullEventBus : public DomainEventBus {
  public:
    void emit(std::unique_ptr<DomainEvent>) override {}
};

} // namespace porytiles2
```

#### 1.2 Domain-Specific Events

```c++
// File: Porytiles2/include/porytiles2/domain/events/CompilationEvents.hpp

namespace porytiles2 {

/**
 * @brief Event emitted when tileset compilation begins
 */
struct TilesetCompilationStarted : public DomainEvent {
    std::string tileset_name;
    std::size_t input_tile_count;
    CompilationMode mode;

    [[nodiscard]] std::string event_name() const override {
        return "TilesetCompilationStarted";
    }
};

/**
 * @brief Event emitted when tile deduplication completes
 */
struct TileDeduplicationCompleted : public DomainEvent {
    std::size_t original_count;
    std::size_t deduplicated_count;
    std::chrono::milliseconds duration;

    [[nodiscard]] std::string event_name() const override {
        return "TileDeduplicationCompleted";
    }
};

/**
 * @brief Event emitted when palette generation fails
 */
struct PaletteGenerationFailed : public DomainEvent {
    std::string error_message;
    std::vector<Color> problematic_colors;
    PaletteConstraints constraints;

    [[nodiscard]] std::string event_name() const override {
        return "PaletteGenerationFailed";
    }
};

/**
 * @brief Event emitted when animation frame is processed
 */
struct AnimationFrameProcessed : public DomainEvent {
    std::size_t frame_index;
    std::size_t tile_count;
    AnimationType animation_type;

    [[nodiscard]] std::string event_name() const override {
        return "AnimationFrameProcessed";
    }
};

/**
 * @brief Event emitted when metatile attributes are validated
 */
struct MetatileAttributesValidated : public DomainEvent {
    std::size_t metatile_count;
    std::size_t validation_warnings;
    std::vector<std::string> warning_details;

    [[nodiscard]] std::string event_name() const override {
        return "MetatileAttributesValidated";
    }
};

/**
 * @brief Event emitted when a performance threshold is exceeded
 */
struct PerformanceThresholdExceeded : public DomainEvent {
    std::string operation;
    std::chrono::milliseconds actual_duration;
    std::chrono::milliseconds threshold;

    [[nodiscard]] std::string event_name() const override {
        return "PerformanceThresholdExceeded";
    }
};

} // namespace porytiles2
```

### 2. Infrastructure Layer Components

#### 2.1 Event Bus Implementation

```c++
// File: Porytiles2/include/porytiles2/infrastructure/events/AsyncEventBus.hpp

namespace porytiles2 {

/**
 * @brief Thread-safe asynchronous event bus implementation
 */
class AsyncEventBus : public DomainEventBus {
  public:
    using EventHandler = std::function<void(const DomainEvent&)>;
    
    AsyncEventBus();
    ~AsyncEventBus();

    /**
     * @brief Start the event processing thread
     */
    void start();

    /**
     * @brief Stop the event processing thread
     */
    void stop();

    /**
     * @brief Register a handler for a specific event type
     */
    template<typename EventType>
    void register_handler(std::function<void(const EventType&)> handler);

    /**
     * @brief Register a generic handler for all events
     */
    void register_universal_handler(EventHandler handler);

    void emit(std::unique_ptr<DomainEvent> event) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace porytiles2
```

```c++
// File: Porytiles2/lib/infrastructure/events/AsyncEventBus.cpp

namespace porytiles2 {

struct AsyncEventBus::Impl {
    std::queue<std::unique_ptr<DomainEvent>> event_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::atomic<bool> running{false};
    std::thread worker_thread;
    
    // Type-erased handlers
    std::unordered_map<std::string, std::vector<std::function<void(const DomainEvent&)>>> handlers;
    std::vector<EventHandler> universal_handlers;
    std::mutex handlers_mutex;

    void process_events() {
        while (running) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] { return !event_queue.empty() || !running; });

            while (!event_queue.empty()) {
                auto event = std::move(event_queue.front());
                event_queue.pop();
                lock.unlock();

                dispatch_event(*event);
                
                lock.lock();
            }
        }
    }

    void dispatch_event(const DomainEvent& event) {
        std::lock_guard<std::mutex> lock(handlers_mutex);
        
        // Dispatch to type-specific handlers
        auto it = handlers.find(event.event_name());
        if (it != handlers.end()) {
            for (const auto& handler : it->second) {
                try {
                    handler(event);
                } catch (const std::exception& e) {
                    // Log handler exception but don't propagate
                    std::cerr << "Event handler exception: " << e.what() << std::endl;
                }
            }
        }
        
        // Dispatch to universal handlers
        for (const auto& handler : universal_handlers) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                std::cerr << "Universal handler exception: " << e.what() << std::endl;
            }
        }
    }
};

AsyncEventBus::AsyncEventBus() : impl_{std::make_unique<Impl>()} {}

AsyncEventBus::~AsyncEventBus() {
    stop();
}

void AsyncEventBus::start() {
    if (!impl_->running) {
        impl_->running = true;
        impl_->worker_thread = std::thread([this] { impl_->process_events(); });
    }
}

void AsyncEventBus::stop() {
    if (impl_->running) {
        impl_->running = false;
        impl_->queue_cv.notify_all();
        if (impl_->worker_thread.joinable()) {
            impl_->worker_thread.join();
        }
    }
}

template<typename EventType>
void AsyncEventBus::register_handler(std::function<void(const EventType&)> handler) {
    std::lock_guard<std::mutex> lock(impl_->handlers_mutex);
    
    auto wrapper = [handler](const DomainEvent& event) {
        if (auto* typed_event = dynamic_cast<const EventType*>(&event)) {
            handler(*typed_event);
        }
    };
    
    EventType dummy;
    impl_->handlers[dummy.event_name()].push_back(wrapper);
}

void AsyncEventBus::register_universal_handler(EventHandler handler) {
    std::lock_guard<std::mutex> lock(impl_->handlers_mutex);
    impl_->universal_handlers.push_back(handler);
}

void AsyncEventBus::emit(std::unique_ptr<DomainEvent> event) {
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->event_queue.push(std::move(event));
    }
    impl_->queue_cv.notify_one();
}

} // namespace porytiles2
```

#### 2.2 Diagnostic Event Handlers

```c++
// File: Porytiles2/include/porytiles2/infrastructure/diagnostics/DiagnosticEventHandler.hpp

namespace porytiles2 {

/**
 * @brief Handles domain events and converts them to diagnostic output
 */
class DiagnosticEventHandler {
  public:
    explicit DiagnosticEventHandler(std::shared_ptr<DiagnosticSystem> diagnostics);

    void handle_compilation_started(const TilesetCompilationStarted& event);
    void handle_deduplication_completed(const TileDeduplicationCompleted& event);
    void handle_palette_generation_failed(const PaletteGenerationFailed& event);
    void handle_animation_frame_processed(const AnimationFrameProcessed& event);
    void handle_attributes_validated(const MetatileAttributesValidated& event);
    void handle_performance_threshold_exceeded(const PerformanceThresholdExceeded& event);

    /**
     * @brief Register all handlers with the event bus
     */
    void register_with_bus(AsyncEventBus& bus);

  private:
    std::shared_ptr<DiagnosticSystem> diagnostics_;
    
    DiagnosticLevel map_event_to_level(const DomainEvent& event);
};

} // namespace porytiles2
```

### 3. Application Layer Integration

#### 3.1 Event Bus Factory and Configuration

```c++
// File: Porytiles2/include/porytiles2/application/events/EventBusFactory.hpp

namespace porytiles2 {

/**
 * @brief Factory for creating and configuring event buses
 */
class EventBusFactory {
  public:
    enum class BusType {
        NULL_BUS,      // For testing
        SYNC_BUS,      // Synchronous processing
        ASYNC_BUS      // Asynchronous with worker thread
    };

    struct Configuration {
        BusType type = BusType::ASYNC_BUS;
        bool enable_diagnostics = true;
        bool enable_metrics = false;
        std::size_t max_queue_size = 10000;
        std::chrono::milliseconds processing_interval{10};
    };

    static std::unique_ptr<DomainEventBus> create(
        const Configuration& config,
        std::shared_ptr<DiagnosticSystem> diagnostics = nullptr
    );
};

} // namespace porytiles2
```

#### 3.2 Service Integration Example

```c++
// File: Example of how domain services would use the event bus

namespace porytiles2 {

class TilesetCompiler {
  private:
    std::shared_ptr<DomainEventBus> event_bus_;
    
  public:
    explicit TilesetCompiler(std::shared_ptr<DomainEventBus> event_bus)
        : event_bus_{event_bus ? event_bus : std::make_shared<NullEventBus>()} {}

    CompilationResult compile(const TilesetInput& input) {
        // Emit compilation started event
        auto start_event = std::make_unique<TilesetCompilationStarted>();
        start_event->tileset_name = input.name();
        start_event->input_tile_count = input.tiles().size();
        start_event->mode = input.compilation_mode();
        event_bus_->emit(std::move(start_event));

        // Perform deduplication
        auto dedup_start = std::chrono::steady_clock::now();
        auto dedup_result = deduplicate_tiles(input.tiles());
        auto dedup_duration = std::chrono::steady_clock::now() - dedup_start;

        // Emit deduplication completed event
        auto dedup_event = std::make_unique<TileDeduplicationCompleted>();
        dedup_event->original_count = input.tiles().size();
        dedup_event->deduplicated_count = dedup_result.unique_tiles.size();
        dedup_event->duration = std::chrono::duration_cast<std::chrono::milliseconds>(dedup_duration);
        event_bus_->emit(std::move(dedup_event));

        // Check performance threshold
        if (dedup_event->duration > std::chrono::milliseconds{1000}) {
            auto perf_event = std::make_unique<PerformanceThresholdExceeded>();
            perf_event->operation = "tile_deduplication";
            perf_event->actual_duration = dedup_event->duration;
            perf_event->threshold = std::chrono::milliseconds{1000};
            event_bus_->emit(std::move(perf_event));
        }

        // Continue compilation...
    }
};

} // namespace porytiles2
```

## Threading Model

### Option 1: Single Worker Thread (Recommended)
- One dedicated thread pulls events from the queue and dispatches to handlers
- Simple to implement and reason about
- Sufficient for most diagnostic needs
- Handlers execute sequentially, avoiding concurrency issues

### Option 2: Thread Pool
- Multiple worker threads for parallel event processing
- Better throughput for high-volume events
- More complex, requires careful handler design for thread safety
- Consider if profiling shows single thread is a bottleneck

## Implementation Phases

### Phase 1: Core Infrastructure (Week 1)
1. Implement DomainEvent base class
2. Implement DomainEventBus interface
3. Create NullEventBus for testing
4. Define initial set of domain events

### Phase 2: Async Event Bus (Week 2)
1. Implement AsyncEventBus with single worker thread
2. Add thread-safe queue management
3. Implement handler registration and dispatch
4. Create unit tests for event bus

### Phase 3: Diagnostic Integration (Week 3)
1. Create DiagnosticEventHandler
2. Implement handlers for each event type
3. Integrate with existing DiagnosticSystem
4. Add configuration support

### Phase 4: Service Integration (Week 4)
1. Update domain services to accept event bus
2. Add event emission at key points
3. Create application-layer wiring
4. Integration testing

### Phase 5: Optimization and Polish (Week 5)
1. Performance profiling
2. Add metrics collection
3. Implement event filtering/sampling
4. Documentation and examples

## Testing Strategy

### Unit Tests
```c++
TEST(AsyncEventBusTest, EmitsAndHandlesEvents) {
    auto bus = std::make_unique<AsyncEventBus>();
    bus->start();
    
    std::atomic<bool> handled{false};
    bus->register_handler<TilesetCompilationStarted>(
        [&handled](const TilesetCompilationStarted&) {
            handled = true;
        }
    );
    
    auto event = std::make_unique<TilesetCompilationStarted>();
    bus->emit(std::move(event));
    
    // Wait for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    
    EXPECT_TRUE(handled);
}
```

### Integration Tests
- Test full compilation flow with event bus
- Verify diagnostic output matches events
- Test error scenarios and exception handling
- Performance benchmarks

## Performance Considerations

### Event Queue Management
- Use bounded queue to prevent memory issues
- Implement overflow strategies (drop oldest, block producer)
- Consider event sampling for high-frequency events

### Handler Performance
- Keep handlers lightweight
- Offload heavy processing to separate threads if needed
- Monitor handler execution time

### Memory Management
- Use unique_ptr for event ownership
- Consider object pools for high-frequency events
- Profile memory usage under load

## Configuration Options

```c++
// Example configuration file support
{
    "events": {
        "enabled": true,
        "bus_type": "async",
        "max_queue_size": 10000,
        "worker_threads": 1,
        "diagnostics": {
            "enabled": true,
            "level_filter": "info",
            "include_events": ["*"],
            "exclude_events": ["AnimationFrameProcessed"]
        },
        "metrics": {
            "enabled": false,
            "export_interval_ms": 5000
        }
    }
}
```

## Migration Path

For existing code:
1. Add NullEventBus by default (no behavior change)
2. Gradually add event emissions to services
3. Enable real event bus in new deployments
4. Remove old diagnostic code once events are proven

## Future Enhancements

### Event Persistence
- Option to persist events to disk
- Event replay for debugging
- Audit trail generation

### Event Correlation
- Track related events across operations
- Generate operation traces
- Support distributed tracing standards

### Dynamic Configuration
- Runtime enable/disable of specific events
- Dynamic handler registration
- Hot-reload of configuration

### Analytics Integration
- Export events to external systems
- Support for OpenTelemetry
- Custom metrics derivation

## Conclusion

This implementation plan provides a robust, scalable foundation for diagnostic event handling in Porytiles2. The domain events pattern maintains clean architecture boundaries while providing rich diagnostic capabilities. The phased approach allows for incremental implementation and testing, minimizing risk while delivering value early.

The async event bus with worker thread provides good performance without excessive complexity. The infrastructure is extensible, allowing for future enhancements as needs evolve.
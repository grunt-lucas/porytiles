# Domain Layer Diagnostic Patterns for DDD Applications

## Overview

This document outlines several architectural patterns for integrating diagnostics and logging into a domain-driven design (DDD) application without violating the principle that the domain layer should remain pure and focused on business logic.

## The Challenge

In a properly layered DDD architecture:
- The **domain layer** contains pure business logic and should not depend on infrastructure concerns
- The **infrastructure layer** contains technical implementations like logging and diagnostics
- The **application layer** orchestrates use cases by coordinating domain and infrastructure services

The challenge is enabling rich diagnostic information from domain operations without creating inappropriate dependencies.

## Pattern Options

### Option 1: Domain Events + Event Handlers

The domain emits events that describe what happened in business terms. Infrastructure handlers translate these into diagnostics.

**Domain Layer:**
```c++
// Domain event
struct CreditLimitExceeded {
    CustomerId customerId;
    Money orderAmount;
    Money creditLimit;
    Timestamp occurredAt;
};

// Domain service
class OrderProcessingService {
private:
    std::shared_ptr<DomainEventBus> eventBus_;
    
public:
    Result<Order> processOrder(const Order& order, const Customer& customer) {
        if (order.totalAmount() > customer.creditLimit()) {
            // Emit event instead of logging
            eventBus_->emit(CreditLimitExceeded{
                .customerId = customer.id(),
                .orderAmount = order.totalAmount(),
                .creditLimit = customer.creditLimit(),
                .occurredAt = Clock::now()
            });
            return Result<Order>::failure(ErrorCode::CreditLimitExceeded);
        }
        // ... more business logic
    }
};
```

**Infrastructure Layer:**
```c++
class DiagnosticEventHandler {
private:
    std::shared_ptr<DiagnosticSystem> diagnostics_;
    
public:
    void handle(const CreditLimitExceeded& event) {
        diagnostics_->log(
            DiagnosticLevel::Warning,
            "Customer {} attempted order of {} exceeding credit limit of {}",
            event.customerId, event.orderAmount, event.creditLimit
        );
    }
    
    void handle(const InventoryDepleted& event) {
        diagnostics_->log(
            DiagnosticLevel::Critical,
            "Product {} inventory depleted during order processing",
            event.productId
        );
    }
};
```

**Pros:**
- Domain remains completely pure
- Events are reusable for other purposes (audit, integration)
- Natural decoupling between what happened and how it's logged

**Cons:**
- Requires event infrastructure
- Can lead to event proliferation
- Asynchronous by nature (may complicate immediate diagnostics)

### Option 2: Rich Result Types with Context

Domain operations return detailed results that the application layer interprets for diagnostics.

**Domain Layer:**
```c++
enum class NotificationType {
    Validation,
    BusinessRule,
    Warning,
    StateChange
};

struct DomainNotification {
    NotificationType type;
    std::string code;
    std::map<std::string, std::any> metadata;
    std::string message;
};

template<typename T>
class DomainResult {
private:
    std::optional<T> value_;
    std::vector<DomainNotification> notifications_;
    bool success_;
    
public:
    void addNotification(NotificationType type, 
                        std::string code, 
                        std::map<std::string, std::any> metadata,
                        std::string message = "") {
        notifications_.emplace_back(DomainNotification{
            .type = type,
            .code = code,
            .metadata = metadata,
            .message = message
        });
    }
    
    const std::vector<DomainNotification>& notifications() const { 
        return notifications_; 
    }
    
    bool isSuccess() const { return success_; }
    const T& value() const { return *value_; }
};

// Domain service using rich results
class OrderValidationService {
public:
    DomainResult<Order> validateOrder(const Order& order, const Customer& customer) {
        DomainResult<Order> result;
        
        if (order.items().empty()) {
            result.addNotification(
                NotificationType::Validation,
                "order.empty",
                {{"orderId", order.id()}},
                "Order contains no items"
            );
        }
        
        if (order.totalAmount() > customer.creditLimit()) {
            result.addNotification(
                NotificationType::BusinessRule,
                "credit.limit.exceeded",
                {
                    {"customerId", customer.id()},
                    {"orderAmount", order.totalAmount()},
                    {"creditLimit", customer.creditLimit()}
                },
                "Order amount exceeds customer credit limit"
            );
        }
        
        for (const auto& item : order.items()) {
            if (!inventory_.isAvailable(item.productId(), item.quantity())) {
                result.addNotification(
                    NotificationType::BusinessRule,
                    "inventory.insufficient",
                    {
                        {"productId", item.productId()},
                        {"requested", item.quantity()},
                        {"available", inventory_.getQuantity(item.productId())}
                    }
                );
            }
        }
        
        return result;
    }
};
```

**Application Layer:**
```c++
class ProcessOrderUseCase {
private:
    std::shared_ptr<DiagnosticSystem> diagnostics_;
    std::shared_ptr<OrderValidationService> validationService_;
    
public:
    void execute(const OrderRequest& request) {
        auto validationResult = validationService_->validateOrder(order, customer);
        
        // Translate domain notifications to diagnostics
        for (const auto& notification : validationResult.notifications()) {
            DiagnosticLevel level = mapNotificationTypeToLevel(notification.type);
            diagnostics_->logStructured(
                level,
                notification.code,
                notification.message,
                notification.metadata
            );
        }
        
        if (!validationResult.isSuccess()) {
            return;
        }
        
        // Continue processing...
    }
    
private:
    DiagnosticLevel mapNotificationTypeToLevel(NotificationType type) {
        switch (type) {
            case NotificationType::Validation:
                return DiagnosticLevel::Info;
            case NotificationType::BusinessRule:
                return DiagnosticLevel::Warning;
            case NotificationType::Warning:
                return DiagnosticLevel::Warning;
            case NotificationType::StateChange:
                return DiagnosticLevel::Debug;
        }
    }
};
```

**Pros:**
- Domain remains pure but informative
- Application layer has full control over diagnostic interpretation
- Notifications can be accumulated across multiple operations
- Works well with validation scenarios

**Cons:**
- Can make domain method signatures more complex
- Requires disciplined use of result types throughout
- May lead to verbose domain code

### Option 3: Domain-Defined Notification Interface

Define a minimal interface in the domain that infrastructure implements.

**Domain Layer:**
```c++
// Minimal interface defined in domain
class DomainNotifier {
public:
    virtual ~DomainNotifier() = default;
    
    virtual void notify(const std::string& code, 
                       const std::map<std::string, std::any>& context) = 0;
    
    // Convenience overloads
    virtual void notifyWarning(const std::string& code, 
                              const std::map<std::string, std::any>& context) = 0;
                              
    virtual void notifyError(const std::string& code, 
                            const std::map<std::string, std::any>& context) = 0;
};

// Null object pattern for testing
class NullDomainNotifier : public DomainNotifier {
public:
    void notify(const std::string&, const std::map<std::string, std::any>&) override {}
    void notifyWarning(const std::string&, const std::map<std::string, std::any>&) override {}
    void notifyError(const std::string&, const std::map<std::string, std::any>&) override {}
};

// Domain service using notifier
class PaymentProcessingService {
private:
    std::shared_ptr<DomainNotifier> notifier_;
    
public:
    PaymentProcessingService(std::shared_ptr<DomainNotifier> notifier) 
        : notifier_(notifier ? notifier : std::make_shared<NullDomainNotifier>()) {}
    
    Result<Payment> processPayment(const Payment& payment) {
        if (payment.requiresFraudCheck()) {
            notifier_->notifyWarning(
                "payment.fraud_check_required",
                {
                    {"paymentId", payment.id()},
                    {"amount", payment.amount()},
                    {"method", payment.method()}
                }
            );
            
            auto fraudResult = fraudService_->check(payment);
            if (!fraudResult.passed()) {
                notifier_->notifyError(
                    "payment.fraud_check_failed",
                    {
                        {"paymentId", payment.id()},
                        {"reason", fraudResult.reason()}
                    }
                );
                return Result<Payment>::failure(ErrorCode::FraudCheckFailed);
            }
        }
        
        // Process payment...
    }
};
```

**Infrastructure Layer:**
```c++
class DiagnosticDomainNotifier : public DomainNotifier {
private:
    std::shared_ptr<DiagnosticSystem> diagnostics_;
    std::string contextPrefix_;
    
public:
    void notify(const std::string& code, 
                const std::map<std::string, std::any>& context) override {
        diagnostics_->logStructured(
            DiagnosticLevel::Info,
            contextPrefix_ + "." + code,
            context
        );
    }
    
    void notifyWarning(const std::string& code, 
                       const std::map<std::string, std::any>& context) override {
        diagnostics_->logStructured(
            DiagnosticLevel::Warning,
            contextPrefix_ + "." + code,
            context
        );
    }
    
    void notifyError(const std::string& code, 
                     const std::map<std::string, std::any>& context) override {
        diagnostics_->logStructured(
            DiagnosticLevel::Error,
            contextPrefix_ + "." + code,
            context
        );
    }
};
```

**Pros:**
- Domain can actively participate in diagnostics when needed
- Interface is minimal and focused on domain concerns
- Easy to mock for testing
- Can be selectively applied only where needed

**Cons:**
- Domain has a dependency (even if abstract)
- Can be seen as violating pure domain principles
- Risk of overuse leading to chatty interfaces

### Option 4: Aspect-Oriented Approach (Decorators/Interceptors)

Add diagnostics around domain services without modifying them.

**Domain Layer (Pure):**
```c++
class OrderService {
public:
    virtual ~OrderService() = default;
    virtual Result<Order> placeOrder(const Order& order) = 0;
    virtual Result<Order> cancelOrder(const OrderId& orderId) = 0;
    virtual Result<Order> updateOrder(const Order& order) = 0;
};

class PureOrderService : public OrderService {
public:
    Result<Order> placeOrder(const Order& order) override {
        // Pure business logic only
        if (!order.isValid()) {
            return Result<Order>::failure(ErrorCode::InvalidOrder);
        }
        
        if (!inventory_.reserve(order.items())) {
            return Result<Order>::failure(ErrorCode::InsufficientInventory);
        }
        
        auto placedOrder = order.withStatus(OrderStatus::Placed);
        repository_.save(placedOrder);
        
        return Result<Order>::success(placedOrder);
    }
    
    // Other methods...
};
```

**Infrastructure Layer:**
```c++
class DiagnosticOrderServiceDecorator : public OrderService {
private:
    std::unique_ptr<OrderService> inner_;
    std::shared_ptr<DiagnosticSystem> diagnostics_;
    
public:
    DiagnosticOrderServiceDecorator(
        std::unique_ptr<OrderService> inner,
        std::shared_ptr<DiagnosticSystem> diagnostics)
        : inner_(std::move(inner)), diagnostics_(diagnostics) {}
    
    Result<Order> placeOrder(const Order& order) override {
        auto operation = diagnostics_->beginOperation(
            "order.place", 
            {{"orderId", order.id()}, {"customerId", order.customerId()}}
        );
        
        auto result = inner_->placeOrder(order);
        
        if (result.isSuccess()) {
            operation.complete({
                {"status", "success"},
                {"orderTotal", result.value().totalAmount()}
            });
        } else {
            operation.fail({
                {"errorCode", result.error().code()},
                {"errorMessage", result.error().message()}
            });
            
            // Additional contextual logging based on error type
            if (result.error().code() == ErrorCode::InsufficientInventory) {
                diagnostics_->logWarning(
                    "Inventory shortage detected during order placement",
                    {{"orderId", order.id()}}
                );
            }
        }
        
        return result;
    }
    
    Result<Order> cancelOrder(const OrderId& orderId) override {
        auto operation = diagnostics_->beginOperation(
            "order.cancel",
            {{"orderId", orderId}}
        );
        
        auto result = inner_->cancelOrder(orderId);
        
        if (result.isSuccess()) {
            operation.complete({{"refundAmount", result.value().refundAmount()}});
        } else {
            operation.fail({{"reason", result.error().message()}});
        }
        
        return result;
    }
    
    // Decorate other methods similarly...
};

// Generic interceptor approach
template<typename T>
class DiagnosticInterceptor {
private:
    std::shared_ptr<DiagnosticSystem> diagnostics_;
    
public:
    template<typename Func, typename... Args>
    auto intercept(const std::string& operationName, 
                   Func func, 
                   Args&&... args) -> decltype(func(std::forward<Args>(args)...)) {
        auto operation = diagnostics_->beginOperation(operationName);
        
        try {
            auto result = func(std::forward<Args>(args)...);
            operation.complete();
            return result;
        } catch (const std::exception& e) {
            operation.fail({{"exception", e.what()}});
            throw;
        }
    }
};
```

**Pros:**
- Domain remains completely pure
- Diagnostics can be added/removed without touching domain code
- Cross-cutting concerns handled cleanly
- Can be applied selectively

**Cons:**
- Can obscure the actual implementation
- Limited access to internal domain state
- May require extensive wrapping infrastructure
- Performance overhead from indirection

## Hybrid Recommendation

For most applications, a combination of approaches works best:

1. **Use domain events** for significant business occurrences that naturally map to diagnostic needs
2. **Return rich results** from domain operations with enough context for the app layer to generate diagnostics
3. **Keep complex diagnostic logic in the application layer** where it can orchestrate across multiple domain services
4. **Apply decorators selectively** for cross-cutting operational diagnostics

### Example Hybrid Implementation:

```c++
// Domain layer
class OrderService {
public:
    DomainResult<Order> processOrder(const Order& order) {
        DomainResult<Order> result;
        
        // Validation with rich feedback
        if (!order.isValid()) {
            result.addNotification(
                NotificationType::Validation,
                "order.invalid",
                {{"orderId", order.id()}, {"reason", order.validationError()}}
            );
            return result;
        }
        
        // Business rule check
        auto inventoryCheck = checkInventory(order);
        if (!inventoryCheck.passed()) {
            result.addNotification(
                NotificationType::BusinessRule,
                "inventory.insufficient",
                inventoryCheck.details()
            );
            
            // Also emit event for significant business occurrence
            eventBus_->emit(InventoryShortageDetected{
                .orderId = order.id(),
                .shortages = inventoryCheck.shortages()
            });
        }
        
        // Continue processing...
        return result;
    }
};

// Application layer
class ProcessOrderUseCase {
    void execute(const OrderRequest& request) {
        // Wrap with decorator for operational diagnostics
        auto diagnosticService = make_diagnostic_decorator(
            orderService_, 
            diagnostics_
        );
        
        auto result = diagnosticService->processOrder(order);
        
        // Handle domain notifications
        for (const auto& notification : result.notifications()) {
            handleDomainNotification(notification);
        }
        
        // Application-specific diagnostics
        if (result.requiresManualReview()) {
            diagnostics_->logAlert(
                "Order requires manual review",
                buildReviewContext(order, result)
            );
        }
    }
};
```

## Guidelines for Choosing an Approach

### Use Domain Events When:
- The occurrence is a significant business event
- Multiple systems need to react to the occurrence
- You need audit trails or event sourcing
- The diagnostic is a side effect of something important happening

### Use Rich Results When:
- You need to accumulate multiple issues/warnings
- The operation naturally produces metadata useful for diagnostics
- You want the application layer to control diagnostic formatting
- You're already using result types for error handling

### Use Domain Notification Interface When:
- The domain needs to actively report progress or state changes
- You need fine-grained diagnostic points within complex algorithms
- The diagnostic messages require domain expertise to formulate
- You can accept the abstract dependency

### Use Decorators/Interceptors When:
- You need operational metrics (timing, success rates)
- The diagnostics are cross-cutting concerns
- You want to add/remove diagnostics without code changes
- You need to instrument third-party domain implementations

## Testing Considerations

Each approach has different testing implications:

```c++
// Testing with events
TEST(OrderServiceTest, EmitsEventOnCreditLimitExceeded) {
    auto eventBus = std::make_shared<TestEventBus>();
    auto service = OrderService(eventBus);
    
    service.processOrder(highValueOrder, lowCreditCustomer);
    
    ASSERT_TRUE(eventBus->hasEvent<CreditLimitExceeded>());
}

// Testing with rich results
TEST(OrderServiceTest, ReturnsNotificationOnValidationFailure) {
    auto result = service.validateOrder(invalidOrder);
    
    ASSERT_FALSE(result.isSuccess());
    ASSERT_EQ(result.notifications().size(), 1);
    ASSERT_EQ(result.notifications()[0].code, "order.invalid");
}

// Testing with notification interface
TEST(PaymentServiceTest, NotifiesFraudCheckRequired) {
    auto mockNotifier = std::make_shared<MockDomainNotifier>();
    auto service = PaymentService(mockNotifier);
    
    EXPECT_CALL(*mockNotifier, 
        notifyWarning("payment.fraud_check_required", _))
        .Times(1);
    
    service.processPayment(highRiskPayment);
}

// Testing with decorators
TEST(DiagnosticDecoratorTest, LogsOperationFailure) {
    auto innerService = std::make_unique<MockOrderService>();
    auto diagnostics = std::make_shared<TestDiagnosticSystem>();
    auto decorator = DiagnosticOrderServiceDecorator(
        std::move(innerService), 
        diagnostics
    );
    
    EXPECT_CALL(*innerService, placeOrder(_))
        .WillOnce(Return(Result<Order>::failure(ErrorCode::InvalidOrder)));
    
    decorator.placeOrder(order);
    
    ASSERT_TRUE(diagnostics->hasLogEntry(DiagnosticLevel::Error));
}
```

## Conclusion

The choice of pattern depends on your specific requirements:
- **Purity requirement**: How important is keeping the domain completely free of infrastructure concerns?
- **Diagnostic granularity**: How detailed do your diagnostics need to be?
- **Performance constraints**: Can you afford the overhead of events or decorators?
- **Team preferences**: What patterns is your team comfortable with?

Most successful implementations use a combination of these patterns, applying each where it provides the most value with the least complexity.

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

```json
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

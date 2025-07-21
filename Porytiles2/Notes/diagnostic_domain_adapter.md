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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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

```cpp
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

```cpp
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

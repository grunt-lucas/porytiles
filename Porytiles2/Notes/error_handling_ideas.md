# Error Handling System Design

## Requirements

The current error handling in Porytiles2 uses `Result<T, std::string>` (an alias for `std::expected<T, std::string>`). While elegant, passing plain strings around is not robust enough. We need a more fully-featured error handling system with the following key requirements:

- Support for arbitrary classes/structs as error types, where the type is defined close to the code that uses it. These types can define internal enum classes to denote the various error types.

- A trace system, so that results are stackable. That way, high-level code can display a full error trace showing the root cause, all the way up the chain.

- All error types should define a method called "details" which returns a fmt::format template string. This template string may contain "{}" which fmt::format will fill in with other members of the error type. E.g.
```c++
struct ImageLoadError {
    enum class Type { file_not_found, unsupported_channel_count, other_load_error };
    
    Type type_;
    std::string filename_;
    
    struct ChannelCountParams {
        int channel_count_;
    };
    
    struct OtherLoadErrorParams {
        std::string load_error_;
    };
    
    std::variant<std::monostate, ChannelCountParams, OtherLoadErrorParams> params_;
    
    std::string details() {
        switch (type_) {
            case Type::file_not_found:
                return "{}: not found";
            case Type::unsupported_channel_count:
                return "{}: unsupported channel count: {}";
            case Type::other_load_error:
                return "{}: could not be loaded: {}";
        }
    }
};
```

- A configurable printer handler can receive specific error types and print them in a custom way, i.e. they may choose to format some of the parameters using bold or italic if the output is to a TTY, etc

### Desired Ergonomics: A Code Example

```c++
TraceableResult<Image<Rgba32>, ImageLoadError> load_image_from_file(const std::filesystem::path &path) const
{
    using enum ImageLoadError::Type;

    if (!exists(path)) {
        return std::unexpected{ImageLoadError{.type_ = file_not_found, .filename = path.string(), .params_ = std::monostate{}}};
    }
    
    CImg<std::uint8_t> cimg_png{};
    const auto path_c_str = path.c_str();
    try {
        cimg_png.assign(path_c_str);
    }
    catch (const CImgException &e) {
        return std::unexpected{ImageLoadError{.type_ = other_load_error, .filename = path.string(), .params_ = OtherLoadErrorParams{.load_error_ = e.what()}}};
    }

    if (cimg_png.spectrum() != 3 && cimg_png.spectrum() != 4) {
        return std::unexpected{
            ImageLoadError{.type_ = unsupported_channel_count, .filename = path.string(), .params_ = ChannelCountParams{.channel_count_ = cimg_png.spectrum()}}
        };
    }

    // do the rest of the Image loading logic here
}

struct TilesetLoadError {
    enum class Type { bad_image, bad_palette, bad_attribute };

    Type type_;

    std::string details() { 
        switch (type_) {
            case Type::bad_image:
                return "bad tileset image";
            case Type::bad_palette:
                return "bad tileset palette";
            case Type::bad_attribute:
                return "bad tileset attribute";
        }
    }
}

TraceableResult<Tileset, TilesetLoadError> load_tileset(const std::string &name) {
    using enum TilesetLoadError::Type;

    // Load other stuff like the palettes, attributes, etc

    // Ok, now to load the image
    auto image_result = load_image_from_file(get_file_path());
    if (!image_result.has_value()) {
        return TraceableResult<Tileset, TilesetLoadError>::chain(TilesetLoadError{.type_ = bad_image}, image_result);
    }

    // otherwise, use the result and continue with loading the tileset
}

void load_tileset_usecase() {
    auto result = load_tileset("foo");

    if (!result.has_value()) {
        // this would print the full trace, i.e. the TilesetLoadError and the underlying ImageLoadError
        diagnostic_printer.emit_trace(result);
        return;
    }

    // do something with the good result
}
```

## Proposed Implementation

### Approach 1: Type-Erased (std::any)

```c++
#include <any>
#include <vector>
#include <expected>
#include <typeindex>
#include <functional>

namespace porytiles2 {

// Base traceable result that stores errors as std::any
template<typename T, typename E>
class TraceableResult {
  private:
    std::expected<T, E> result_;
    std::vector<std::any> error_trace_;
    
  public:
    // Construct from expected
    TraceableResult(std::expected<T, E> result) : result_{std::move(result)} {
        if (!result_.has_value()) {
            error_trace_.push_back(result_.error());
        }
    }
    
    // Add a cause to the error trace
    template<typename CauseError>
    void add_cause(const CauseError& cause) {
        error_trace_.push_back(cause);
    }
    
    [[nodiscard]] bool has_value() const { return result_.has_value(); }
    [[nodiscard]] T& value() { return result_.value(); }
    [[nodiscard]] const T& value() const { return result_.value(); }
    [[nodiscard]] const E& error() const { return result_.error(); }
    [[nodiscard]] const std::vector<std::any>& trace() const { return error_trace_; }
};

// Diagnostic printer for type-erased approach
class DiagnosticPrinter {
  private:
    bool is_tty_;
    std::map<std::type_index, std::function<std::string(const std::any&, bool)>> formatters_;
    
  public:
    DiagnosticPrinter(bool is_tty) : is_tty_{is_tty} {}
    
    // Register a formatter for a specific error type
    template<typename ErrorType>
    void register_formatter(std::function<std::string(const ErrorType&, bool)> formatter) {
        formatters_[std::type_index(typeid(ErrorType))] = 
            [formatter](const std::any& err, bool use_formatting) {
                return formatter(std::any_cast<const ErrorType&>(err), use_formatting);
            };
    }
    
    // Emit the full error trace
    template<typename T, typename E>
    void emit_trace(const TraceableResult<T, E>& result) {
        for (const auto& error : result.trace()) {
            auto type_idx = std::type_index(error.type());
            if (formatters_.count(type_idx)) {
                std::cout << formatters_[type_idx](error, is_tty_) << "\n";
            } else {
                std::cout << "Unknown error type\n";
            }
        }
    }
};

} // namespace porytiles2
```

**Pros:**
- Maximum flexibility - can store any error type
- No inheritance required for error types
- Errors remain simple structs
- Can add new error types without modifying existing code

**Cons:**
- Runtime type checking overhead
- Loss of compile-time type safety
- Need to register formatters for each error type
- std::any_cast can throw if types don't match
- Harder to debug (type information hidden)

---

### Approach 2: Base Class/Interface

```c++
#include <memory>
#include <vector>
#include <expected>

namespace porytiles2 {

// Base error interface
class ErrorBase {
  public:
    virtual ~ErrorBase() = default;
    [[nodiscard]] virtual std::string details() const = 0;
    [[nodiscard]] virtual std::string format(const std::map<std::string, std::string>& style_map) const = 0;
    // No clone needed for move-only semantics
};

// Example error type - directly inherit from ErrorBase
struct ImageLoadError : ErrorBase {
    enum class Type { file_not_found, unsupported_channel_count, other_load_error };
    
    Type type_;
    std::string filename_;
    
    struct ChannelCountParams {
        int channel_count_;
    };
    
    struct OtherLoadErrorParams {
        std::string load_error_;
    };
    
    std::variant<std::monostate, ChannelCountParams, OtherLoadErrorParams> params_;
    
    [[nodiscard]] std::string details() const override {
        switch (type_) {
            case Type::file_not_found:
                return fmt::format("{}: not found", filename_);
            case Type::unsupported_channel_count:
                return fmt::format("{}: unsupported channel count: {}", 
                    filename_, std::get<ChannelCountParams>(params_).channel_count_);
            case Type::other_load_error:
                return fmt::format("{}: could not be loaded: {}", 
                    filename_, std::get<OtherLoadErrorParams>(params_).load_error_);
        }
    }
    
    [[nodiscard]] std::string format(const std::map<std::string, std::string>& style_map) const override {
        // Apply TTY formatting if available
        std::string result = details();
        if (style_map.count("bold")) {
            result = style_map.at("bold") + result + style_map.at("reset");
        }
        return result;
    }
};

// Traceable result for base class approach
template<typename T>
class TraceableResult {
  private:
    std::optional<T> value_;
    std::vector<std::unique_ptr<ErrorBase>> error_trace_;
    
  public:
    TraceableResult(T value) : value_{std::move(value)} {}
    
    TraceableResult(std::unique_ptr<ErrorBase> error) {
        error_trace_.push_back(std::move(error));
    }
    
    // Move-only semantics
    TraceableResult(TraceableResult&&) = default;
    TraceableResult& operator=(TraceableResult&&) = default;
    TraceableResult(const TraceableResult&) = delete;
    TraceableResult& operator=(const TraceableResult&) = delete;
    
    void add_cause(std::unique_ptr<ErrorBase> cause) {
        error_trace_.push_back(std::move(cause));
    }
    
    [[nodiscard]] bool has_value() const { return value_.has_value(); }
    [[nodiscard]] T& value() { return value_.value(); }
    [[nodiscard]] const T& value() const { return value_.value(); }
    [[nodiscard]] const std::vector<std::unique_ptr<ErrorBase>>& trace() const { return error_trace_; }
};

// Diagnostic printer for base class approach
class DiagnosticPrinter {
  private:
    bool is_tty_;
    std::map<std::string, std::string> style_map_;
    
  public:
    DiagnosticPrinter(bool is_tty) : is_tty_{is_tty} {
        if (is_tty) {
            style_map_["bold"] = "\033[1m";
            style_map_["italic"] = "\033[3m";
            style_map_["reset"] = "\033[0m";
        }
    }
    
    template<typename T>
    void emit_trace(const TraceableResult<T>& result) {
        for (const auto& error : result.trace()) {
            std::cout << error->format(style_map_) << "\n";
        }
    }
};

} // namespace porytiles2
```

**Pros:**
- Type-safe at compile time for the interface
- Polymorphic behavior through virtual functions
- Each error type can implement its own formatting logic
- Clean separation of concerns

**Cons:**
- Requires inheritance (all errors must derive from base)
- Virtual function overhead
- Need unique_ptr for polymorphic storage (heap allocation)
- More boilerplate (CRTP for clone, virtual functions)
- Move-only semantics due to unique_ptr

---

### Approach 3: Variant-Based

```c++
#include <variant>
#include <vector>
#include <expected>

#include "porytiles2/domain/errors/image_load_error.hpp"
#include "porytiles2/domain/errors/tileset_load_error.hpp"
#include "porytiles2/domain/errors/palette_error.hpp"
// ...and so on for every error type in the project

namespace porytiles2 {
// Define variant of all possible errors
using ErrorVariant = std::variant<
    ImageLoadError,
    TilesetLoadError,
    PaletteError
    // ... more error types
>;

// Error types remain simple structs
struct ImageLoadError {
    enum class Type { file_not_found, unsupported_channel_count, other_load_error };
    
    Type type_;
    std::string filename_;
    
    struct ChannelCountParams {
        int channel_count_;
    };
    
    struct OtherLoadErrorParams {
        std::string load_error_;
    };
    
    std::variant<std::monostate, ChannelCountParams, OtherLoadErrorParams> params_;
    
    // Returns format template string, NOT formatted result
    [[nodiscard]] std::string details() const {
        switch (type_) {
            case Type::file_not_found:
                return "{}: not found";
            case Type::unsupported_channel_count:
                return "{}: unsupported channel count: {}";
            case Type::other_load_error:
                return "{}: could not be loaded: {}";
        }
    }
};

struct TilesetLoadError {
    enum class Type { bad_image, bad_palette, bad_attribute };
    Type type_;
    std::string tileset_name_;
    
    [[nodiscard]] std::string details() const { 
        switch (type_) {
            case Type::bad_image:
                return "bad image for tileset: {}";
            case Type::bad_palette:
                return "bad palette for tileset: {}";
            case Type::bad_attribute:
                return "bad attributes for tileset: {}";
        }
    }
};

struct PaletteError {
    enum class Type { invalid_color_count, bad_format };
    Type type_;
    std::string palette_name_;
    int color_count_;
    
    [[nodiscard]] std::string details() const {
        switch (type_) {
            case Type::invalid_color_count:
                return "palette {} has invalid color count: {}";
            case Type::bad_format:
                return "palette {} has invalid format";
        }
    }
};

// Traceable result using variant
template<typename T, typename E>
class TraceableResult {
  private:
    std::expected<T, E> result_;
    std::vector<ErrorVariant> error_trace_;
    
  public:
    // Ctor for a success value, or for an originating error.
    // To create an originating error, you must use the std::unexpected wrapper to disambiguate.
    TraceableResult(std::expected<T, E> result) : result_{std::move(result)} {
        if (!result_.has_value()) {
            error_trace_.push_back(result_.error());
        }
    }

    // Ctor for chaining an error. The std::unexpected is handled internally to reduce verbosity.
    TraceableResult(const E& error, const auto& cause_result)
          : result_{std::unexpected{error}}
    {
        error_trace_.push_back(result_.error());
        add_cause(cause_result);
    }
    
    template<typename CauseT, typename CauseE>
    [[nodiscard]] static TraceableResult<T, E> chain(E error, const TraceableResult<CauseT, CauseE>& cause) {
        return TraceableResult{error, cause};
    }
    
    template<typename OtherT, typename OtherE>
    void add_cause(const TraceableResult<OtherT, OtherE>& cause_result) {
        // Assumes cause_result does not have a value
        for(const auto& err : cause_result.trace()) {
            error_trace_.push_back(err);
        }
    }
    
    [[nodiscard]] bool has_value() const { return result_.has_value(); }
    [[nodiscard]] T& value() { return result_.value(); }
    [[nodiscard]] const T& value() const { return result_.value(); }
    [[nodiscard]] const E& error() const { return result_.error(); }
    [[nodiscard]] const std::vector<ErrorVariant>& trace() const { return error_trace_; }
};

// Visitor for formatting errors - handles all formatting including TTY styling
class ErrorFormatter {
  private:
    bool is_tty_;
    
  public:
    ErrorFormatter(bool is_tty) : is_tty_{is_tty} {}
    
    template <typename T>
    auto style(const T &t, fmt::text_style ts) const {
        return fmt::styled(t, is_tty_ ? ts : fmt::text_style{});
    }
    
    std::string operator()(const ImageLoadError& err) const {
        switch (err.type_) {
            case ImageLoadError::Type::file_not_found: {
                // Make filename bold
                auto styled_filename = style(err.filename_, fmt::text_style{fmt::emphasis::bold});
                return fmt::format(err.details(), styled_filename);
            }
                
            case ImageLoadError::Type::unsupported_channel_count: {
                // Make filename bold and channel count red
                auto styled_filename = style(err.filename_, fmt::text_style{fmt::emphasis::bold});
                auto channel_count = std::get<ImageLoadError::ChannelCountParams>(err.params_).channel_count_;
                auto styled_channel_count = style(channel_count, fmt::text_style{fmt::fg(fmt::terminal_color::red) | fmt::emphasis::bold});
                return fmt::format(err.details(), styled_filename, styled_channel_count);
            }

            // ... more cases
            default:
                panic("unhandled ImageLoadError type")
        }
    }
    
    // Other error types can be handled similarly...
};

// Diagnostic printer using visitor
class DiagnosticPrinter {
  private:
    bool is_tty_;
    
  public:
    DiagnosticPrinter(bool is_tty) : is_tty_{is_tty} {}
    
    template<typename T, typename E>
    void emit_trace(const TraceableResult<T, E>& result) {
        ErrorFormatter formatter{is_tty_};
        for (const auto& error : result.trace()) {
            std::cout << std::visit(formatter, error) << "\n";
        }
    }
};

} // namespace porytiles2
```

**Pros:**
- Compile-time type safety
- No virtual function overhead
- Visitor pattern allows clean separation of formatting logic
- Errors remain simple value types (no inheritance)
- Visitor must handle all variant types (compile error if missing)
- Stack allocation possible (no heap required)
- Best performance of the three approaches

**Cons:**
- Must know all error types at compile time
- Adding new error types requires modifying the variant
- Can lead to large variant types if many error types exist
- All error types must be complete types when variant is defined, which leads to a "god header"

### Dealing With The God Header Problem
The "god header" is the most significant drawback to this approach, as it increases coupling and can lengthen build times. Unfortunately, `std::variant` requires complete types at the point of definition (it needs to know sizes, alignment, and other type traits), so forward declarations cannot be used. However, we can still minimize the negative effects with two complementary strategies.

#### 1. Keep the Variant Header Focused
Since we cannot use forward declarations with `std::variant`, the "god header" must include all error type definitions. However, we can still minimize its impact by keeping the error types themselves lightweight.

**How to do it:**
1.  Create a new central header, e.g., `porytiles2/domain/errors/error_variant.hpp`.
2.  In this file, include all individual error headers.
3.  Define the `ErrorVariant` using the complete types.
4.  Keep error structs as simple POD types to minimize what gets pulled in.

**Example: `porytiles2/domain/errors/error_variant.hpp`**
```c++
#pragma once

#include <variant>

// Must include all error type definitions - std::variant requires complete types
#include "porytiles2/domain/errors/image_load_error.hpp"
#include "porytiles2/domain/errors/tileset_load_error.hpp"
#include "porytiles2/domain/errors/palette_error.hpp"
// ... add new error headers here

namespace porytiles2 {

// Define the variant using the complete types
using ErrorVariant = std::variant<
    ImageLoadError,
    TilesetLoadError,
    PaletteError
    // ... add new type names here
>;

} // namespace porytiles2
```

**Impact:**
While we cannot avoid including all error definitions, keeping the error types as simple structs (with methods implemented in .cpp files) minimizes the compilation overhead. This is a fundamental tradeoff when using `std::variant` - we accept the coupling in exchange for type safety and performance.

---

#### 2. Implement Error Methods in `.cpp` Files
To make Strategy #1 work, the full definitions of the error structs must be separated from their declarations.

**How to do it:**
1.  In the individual error headers (e.g., `image_load_error.hpp`), define the struct but declare its methods.
2.  Implement those methods in a corresponding `.cpp` file (e.g., `image_load_error.cpp`).

**Example: `image_load_error.hpp`**
```c++
#pragma once
#include <string>
#include <variant>

namespace porytiles2 {

struct ImageLoadError {
    // ... members ...

    // Declare the method, but do not implement it here
    [[nodiscard]] std::string details() const;
};

} // namespace porytiles2
```

**Impact:**
This is standard C++ practice that decouples the interface from the implementation, further reducing what gets pulled into other files during compilation.

---

#### 3. Create a "Compilation Firewall" for the Visitor
The `ErrorFormatter` visitor is the one place that *must* know the full definition of every error type. Isolate this dependency into a single `.cpp` file that acts as a "firewall."

**How to do it:**
1.  Define the `DiagnosticPrinter` class in a header, but only declare the methods that operate on `ErrorVariant`.
2.  In the `.cpp` file for your printer, include all the individual `*_error.hpp` headers and implement the visitor logic.

**Example: `diagnostic_printer.cpp` (The Firewall)**
```c++
#include "porytiles2/infra/diagnostic_printer.hpp"

// This is the ONE place where we include all concrete error types.
// When adding a new error, you only need to add an include here.
#include "porytiles2/domain/errors/image_load_error.hpp"
#include "porytiles2/domain/errors/tileset_load_error.hpp"
#include "porytiles2/domain/errors/palette_error.hpp"

#include <iostream>
#include <fmt/color.h>

namespace porytiles2 {
namespace {
    // The visitor is now an implementation detail hidden in this .cpp file
    struct ErrorVisitor {
        bool is_tty_;
        // ... operator() overloads for ImageLoadError, TilesetLoadError, etc.
    };
}

void DiagnosticPrinter::emit_trace(const std::vector<ErrorVariant>& trace) {
    ErrorVisitor visitor{is_tty_};
    for (const auto& error : trace) {
        std::cout << std::visit(visitor, error) << " caused by -> ";
    }
    std::cout << std::endl;
}

} // namespace porytiles2
```

**Impact:**
When you add a new error type, you will only need to recompile the `.cpp` files that directly create that error and this one `diagnostic_printer.cpp` file. The rest of the project is unaffected. This is the single most effective way to contain the "god header" problem.

### Implement Variant-Based Error Handling System

This plan outlines the steps to replace the current `Result<T, std::string>` with the new variant-based traceable error handling system, as detailed in `Porytiles2/Notes/error_handling_ideas.md`.

#### TODO

-   [ ] **Phase 1: Core Infrastructure**
    -   [ ] 1. Create the "god header" `porytiles2/include/porytiles2/domain/errors/error_variant.hpp` that includes all error type headers (note: forward declarations won't work with std::variant).
    -   [ ] 2. Create the `TraceableResult` class template in `porytiles2/include/porytiles2/domain/traceable_result.hpp`. It will use `ErrorVariant`.
    -   [ ] 3. Create the `DiagnosticPrinter` class and the firewall `.cpp` file.
-   [ ] **Phase 2: Refactor an Existing Error Site**
    -   [ ] 1. Choose a simple, existing use of `Result<T, std::string>` to refactor. A good candidate would be in file loading.
    -   [ ] 2. Define a new error struct for it (e.g., `FileLoadError`).
    -   [ ] 3. Add the new error to `error_variant.hpp`.
    -   [ ] 4. Update the function to return a `TraceableResult` with the new error type.
    -   [ ] 5. Update the `DiagnosticPrinter` to handle the new error type.
    -   [ ] 6. Update the call site to handle the new `TraceableResult`.
-   [ ] **Phase 3: Build and Test**
    -   [ ] 1. Build the project to ensure the new types and refactoring work.
    -   [ ] 2. Run unit and integration tests to verify correctness.
-   [ ] **Phase 4: Review**
    -   [ ] 1. Add a summary of changes for review.

---

## Comparison Summary

| Aspect | Type-Erased (std::any) | Base Class | Variant |
|--------|------------------------|------------|---------|
| Type Safety | Runtime | Compile-time (interface) | Compile-time (full) |
| Performance | Worst (type checking) | Medium (virtual calls) | Best (static dispatch) |
| Flexibility | Add types anytime | Add types anytime | Fixed set of types |
| Memory | Stack or heap | Heap (unique_ptr) | Stack preferred |
| Boilerplate | Register formatters | Virtual functions, CRTP | Visitor cases |
| Error Definition | Simple structs | Inherit from base | Simple structs |
| Debugging | Hardest | Medium | Easiest |

## Recommendation

Based on your requirements and the single-threaded nature of the application, I recommend **Approach 3: Variant-Based** for the following reasons:

1. **Best performance** - No virtual dispatch or runtime type checking
2. **Full compile-time type safety** - Catches errors at compile time
3. **Simple error types** - Errors remain POD structs without inheritance
4. **Clean visitor pattern** - Separates formatting logic from error definitions
5. **Stack allocation** - Better cache locality and no heap overhead

The main drawback (fixed set of error types) is manageable in a compiler project where error types are generally well-defined upfront. The visitor pattern with TTY-aware formatting aligns perfectly with your requirement for conditional formatting.

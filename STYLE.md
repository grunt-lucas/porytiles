```C++
// First include should always be declaration header, if relevant
#include "porytiles2/domain/MyClass.hpp"

// Next, include C++ stdlib headers with angle brackets
#include <string>
#include <vector>

// Next, include project libraries with quotes
#include "gsl/pointers"
#include "yaml-cpp/yaml.h"

// Finally, include other Porytiles headers with quotes
#include "porytiles2/domain/foo.hpp"

// Notice that between each include group, we place an extra newline

// Namespace for all Porytiles2 code should be "porytiles2", never use a child namespace
namespace porytiles2 {

// PascalCase for enum class names
enum class FooBar {
    // snake_case for the actual constants
    foo_value_1,
    foo_value_2
};

// snake_case for global constants
const std::string foo_bar_value_1 = "foo_value_1";

// PascalCase for class names
class MyClass {
  public:  
    MyClass() = default;
    
    // ctor initializer lists always use braced initialization where possible
    // simple ctors can be implemented in the header file
    MyClass(int my_val) : my_val_{my_val} {}
  
    // class constants use snake_case
    const std::string my_class_constant = "my_class_constant";
  
    // Method names are snake_case, parameter names are snake_case
    // Use [[nodiscard]] for methods/functions that return a value
    [[nodiscard]] int compute_something(int accum_value) const;
    
    // Do something complicated to update my_val_
    // This should be implemented in the cpp file
    void update_my_val_with_complex_process(int some_param);
  
    // Simple accessors/mutators also use snake_case, but omit the trailing underscore
    // Simple accessors/mutators can be implemented in the header file
    [[nodiscard]] const std::string &cool_value() const {
        return cool_value_;
    }
    
    [[nodiscard]] int my_val() const {
        return my_val_;
    }

    void my_val(int new_val) {
        my_val_ = new_val;
    }
  
  private:
    // Member variables use snake_case_ with trailing underscore
    std::string cool_value_;
    int my_val_;
};

// cpp file implementations
int MyClass::compute_something(int accum_value) const {
    /*
     * For extended multi-line code comments,
     * prefer the slash-star style like so.
     */
    // Single line comments can use the two-slash style.

    // local variable names are snake_case
    int my_local = 1;
    return my_local + my_val_ + accum_value;
}

} // namespace porytiles2

// Close a namespace with a closing comment like above
```

## Doxygen Comment Style
```C++
// Always use @brief and @details
/**
 * @brief A basic class for for modeling foos.
 *
 * @details
 * The Foo class assumes that your foos are all like bars, but different. Notice that the comment goes all the way to
 * the column limit of 120 before wrapping.
 *
 * @tparam T The type parameter for the foo
 * @invariant Some note would go here
 */
template <typename T>
class Foo {
  public:
   // NOTICE:
   // a blank line between @brief and @details
   // a blank line between @details and the other doc tags
   //
   // IDIOMATIC TAG ORDER:
   // 1. @brief
   // 2. @details
   // 3. @tparam (for templates)
   // 4. @invariant (only relevant for structs/classes, condition that is true at all times in object lifecycle)
   // 5. @param (for parameters)
   // 6. @pre (preconditions - what must be true BEFORE calling)
   // 7. @return (what the function returns)
   // 8. @post (postconditions - what is guaranteed AFTER calling)
   // 9. @note/@warning/@see (if applicable)
   // 10. @todo (for formal documentation of possible upcoming changes)
   //
   // You can use @c for code and @p for parameters.
   //
   // IMPORTANT: Do NOT use @throws/@exception tags
   // This codebase uses panic/abort for unrecoverable errors (like precondition
   // violations) rather than C++ exceptions. Precondition violations should be
   // documented with @pre tags. Panics are not exceptions - they terminate the
   // program and are not catchable/recoverable.

   /**
    * @brief Computes a bar value by applying a factor to a base value.
    *
    * @details
    * This function performs a computation using the provided factor and base value.
    * The function panics if the factor is negative, exceeds the maximum safe value,
    * or if the base is zero. The computation is optimized for positive integers.
    *
    * @tparam ResultType The type to cast the result to (must be numeric).
    * @param factor The factor to use in the computation.
    * @param base The base value to multiply with the factor.
    * @pre @p factor must be non-negative.
    * @pre @p factor must be less than @c MAX_SAFE_FACTOR.
    * @pre @p base must not be zero.
    * @return The computed bar value cast to @c ResultType.
    * @post The returned value is always positive.
    * @post The returned value is less than @c MAX_BAR_VALUE.
    * @note This function is thread-safe.
    * @warning This function may lose precision when casting to smaller numeric types.
    * @see @c compute_baz() for a related computation.
    * @see @c apply_factor() for a simpler version without base parameter.
    * @todo Handle MAX_SAFE_FACTOR more elegantly
    */
    template <typename ResultType>
    ResultType compute_bar(int factor, int base);
};
```

## std::formatter Specializations

When adding `std::formatter<T>` specializations for custom types, the `format()` method
**MUST** use `auto &ctx` — not `std::format_context &ctx`:

```c++
// CORRECT — works with std::formattable concept on all compilers:
auto format(const MyType &val, auto &ctx) const

// BROKEN on Apple Clang / libc++ — std::formattable<MyType, char> evaluates to false:
auto format(const MyType &val, std::format_context &ctx) const
```

This is due to a libc++ implementation issue (LLVM #66466) where the std::formattable concept
tests against an internal context type, not std::format_context directly.

All formatters should delegate to the type's porytiles2::to_string() overload for consistency:

```c++
template <>
struct std::formatter<porytiles2::MyType> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::MyType &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};
```

## Error and Diagnostic Message Style

All user-facing error and diagnostic messages (in `FormattableError`, `PT_TRY_ASSIGN_CHAIN_ERR`,
`diag_->warning()`, `diag_->error()`, etc.) must follow these rules:

1. **Capital first letter** — the message must start with an uppercase letter
2. **Ends with a period `.`** — every message must end with a period
3. **Single quotes and `Style::bold`** around highlightable items (file names, tileset names, keys):
   `FormatParam{tileset_name, Style::bold}` with `'{}'` in the format string
4. **List headers ending with `:`** are acceptable (e.g. `"To resolve:"`, `"Changes present in Porymap assets:"`)
5. **Empty strings** used as separators are fine
6. **Bullet-point sub-items** in multi-line error lists follow their own style

```c++
// CORRECT:
FormattableError{"Tileset '{}' does not exist.", FormatParam{tileset_name, Style::bold}};
FormattableError{"Failed to read metatile_attributes.bin."};
FormattableError{"Failed to open file for writing: '{}'.", FormatParam{path, Style::bold}};

// WRONG — lowercase start:
FormattableError{"tileset save failed"};

// WRONG — missing period:
FormattableError{"Failed to read metatile_attributes.bin"};

// WRONG — raw string concatenation instead of FormatParam:
FormattableError{"failed to pack palettes for tileset " + tileset_.name()};
// Should be:
FormattableError{"Failed to pack palettes for tileset '{}'.", FormatParam{tileset_.name(), Style::bold}};
```

Note about markdown code blocks: when writing multiline C++ code blocks,
use "c++" after the triple backticks. E.g.,
```c++
int main() {
    // This is a multiline c++ code block
    // Notice that the triple backticks are followed by "c++"
    return 0;
}
```

## Preferred Idioms

### Container Membership Checks

Since we target C++23, prefer `contains` over the `find`/`end` iterator pattern for
associative containers (`std::map`, `std::unordered_map`, `std::set`, etc.):

```c++
// CORRECT — C++20/23 style:
if (my_map.contains(key)) {
    const auto &val = my_map.at(key);
    // ...
}

// AVOID — pre-C++20 style:
if (auto it = my_map.find(key); it != my_map.end()) {
    const auto &val = it->second;
    // ...
}
```

**Exception**: If you need the iterator for purposes beyond just accessing the value
(e.g., erasing, modifying in-place, or iterating from that position), the `find` pattern
is still appropriate.

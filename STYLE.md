```C++
// First include should always be declaration header, if relevant
#include "porytiles2/domain/MyClass.hpp"

// Next, include C++ stdlib headers with angle brackets
#include <string>
#include <vector>

// Next, include project libraries with quotes
#include "fmt/format.h"
#include "gsl/pointers"

// Finally, include other Porytiles headers with quotes
#include "porytiles2/domain/Foo.hpp"

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
 * The Foo class assumes that your foos are all like bars, but different.
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
    * @tparam ResultType The type to cast the result to (must be numeric)
    * @param factor The factor to use in the computation
    * @param base The base value to multiply with the factor
    * @pre factor must be non-negative
    * @pre factor must be less than MAX_SAFE_FACTOR
    * @pre base must not be zero
    * @return The computed bar value cast to ResultType
    * @post The returned value is always positive
    * @post The returned value is less than MAX_BAR_VALUE
    * @note This function is thread-safe
    * @warning This function may lose precision when casting to smaller numeric types
    * @see compute_baz() for a related computation
    * @see apply_factor() for a simpler version without base parameter
    * @todo Handle MAX_SAFE_FACTOR more elegantly
    */
    template <typename ResultType>
    ResultType compute_bar(int factor, int base);
};
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

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

// PascalCase for class names
class MyClass {
  public:
    MyClass() = default;
    
    // ctor initializer lists always use braced initialization where possible
    // simple ctors can be implemented in the header file
    MyClass(int my_val) : my_val_{my_val} {}
  
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
 */
class Foo {
  public:
   // NOTICE:
   // a blank line between @brief and @details
   // a blank line between @details and the other doc tags
   /**
    * @brief Computes a bar with a given factor.
    *
    * @details
    * The factor is used to compute the bar.
    *
    * @param factor The factor to use
    * @return The computed bar
    */
    int compute_bar(int factor); 
};
```

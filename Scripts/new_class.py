#!/usr/bin/env python3

"""
Script to create a new C++ class header file.
Usage: python new_class.py <CamelCaseClassName> <parent/path>
"""

import sys
import os
import re


def camel_to_snake(name):
    """Convert CamelCase to snake_case."""
    # Insert underscore before capital letters (except the first one)
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    # Insert underscore before capital letters that are followed by lowercase
    s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
    return s2.lower()


def create_class_header(class_name, header_path):
    """Create a C++ header file with a class definition."""
    # Convert class name to snake_case for filename
    snake_class_name = camel_to_snake(class_name)
    hpp_filename = snake_class_name + '.hpp'
    
    # Create full path
    full_path = os.path.join(header_path, hpp_filename)
    
    # Create header file content
    content = f"""#pragma once

namespace porytiles2 {{

/**
 * @brief Represents a foo.
 */
class {class_name} {{
public:
    {class_name}() = default;
    ~{class_name}() = default;

    [[nodiscard]] int foo() const;

private:
    int foo_{{}};
}};

}} // namespace porytiles2
"""
    
    # Create parent directory if it doesn't exist
    os.makedirs(header_path, exist_ok=True)
    
    # Write the header file
    with open(full_path, 'w') as f:
        f.write(content)
    
    print(f"Created {full_path}")


def create_class_impl(class_name, impl_path, layer_path):
    """Create a C++ cpp file with a class implementation."""
    # Convert class name to snake_case for filename
    snake_class_name = camel_to_snake(class_name)
    cpp_filename = snake_class_name + '.cpp'
    
    # Create full path
    full_path = os.path.join(impl_path, cpp_filename)
    
    # Create impl file content
    content = f"""#include "porytiles2/{layer_path}/{snake_class_name}.hpp"

namespace porytiles2 {{

int {class_name}::foo() const {{
    return foo_;
}}

}} // namespace porytiles2
"""
    
    # Create parent directory if it doesn't exist
    os.makedirs(impl_path, exist_ok=True)
    
    # Write the header file
    with open(full_path, 'w') as f:
        f.write(content)
    
    print(f"Created {full_path}")


def create_class_test(class_name, test_path, layer_path):
    """Create a C++ cpp file with a class test."""
    # Convert class name to snake_case for filename
    snake_class_name = camel_to_snake(class_name)
    cpp_filename = snake_class_name + '_test.cpp'
    
    # Create full path
    full_path = os.path.join(test_path, cpp_filename)
    
    # Create impl file content
    content = f"""#include "gtest/gtest.h"
    
#include "porytiles2/{layer_path}/{snake_class_name}.hpp"

using namespace porytiles2;

TEST({class_name}Tests, FooShouldBeZero) {{
    {class_name} foo{{}};
    EXPECT_EQ(foo.foo(), 0);
}}
"""
    
    # Create parent directory if it doesn't exist
    os.makedirs(test_path, exist_ok=True)
    
    # Write the header file
    with open(full_path, 'w') as f:
        f.write(content)
    
    print(f"Created {full_path}")


def main():
    # Check if running from project root
    if not os.path.isfile('.porytiles-marker-file'):
        print("Error: Script must be run from the main Porytiles project root")
        sys.exit(1)
    
    # Check command line arguments
    if len(sys.argv) != 3:
        print("Usage: ./new_class.py <CamelCaseClassName> <layer/path>")
        sys.exit(1)
    
    class_name = sys.argv[1]
    layer_path = sys.argv[2]
    header_path = "Porytiles2/include/porytiles2/" + layer_path
    impl_path = "Porytiles2/lib/" + layer_path
    test_path = "Porytiles2/tests/unit/" + layer_path
    
    # Validate class name (should start with uppercase letter)
    if not class_name or not class_name[0].isupper():
        print("Error: Class name should start with an uppercase letter")
        sys.exit(1)
    
    # Create the header file
    try:
        create_class_header(class_name, header_path)
    except Exception as e:
        print(f"Error creating header file: {e}")
        sys.exit(1)

    # Create the cpp file
    try:
        create_class_impl(class_name, impl_path, layer_path)
    except Exception as e:
        print(f"Error creating cpp file: {e}")
        sys.exit(1)

    # Create the test file
    try:
        create_class_test(class_name, test_path, layer_path)
    except Exception as e:
        print(f"Error creating test file: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Script to create a new C++ class header file.

Usage: uv run scripts/new_class.py <CamelCaseClassName> <parent/path> [--header-only] [--no-test]
"""

import argparse
import re
import sys
from pathlib import Path


def camel_to_snake(name):
    """Convert CamelCase to snake_case."""
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
    return s2.lower()


def create_class_header(class_name, header_path):
    """Create a C++ header file with a class definition."""
    snake_class_name = camel_to_snake(class_name)
    hpp_filename = snake_class_name + '.hpp'
    full_path = header_path / hpp_filename

    if full_path.exists():
        print(f"file {full_path} already exists")
        return

    content = f"""#pragma once

namespace porytiles {{

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

}} // namespace porytiles
"""

    header_path.mkdir(parents=True, exist_ok=True)
    full_path.write_text(content)
    print(f"Created {full_path}")


def create_class_impl(class_name, impl_path, layer_path):
    """Create a C++ cpp file with a class implementation."""
    snake_class_name = camel_to_snake(class_name)
    cpp_filename = snake_class_name + '.cpp'
    full_path = impl_path / cpp_filename

    if full_path.exists():
        print(f"file {full_path} already exists")
        return

    content = f"""#include "porytiles/{layer_path}/{snake_class_name}.hpp"

namespace porytiles {{

int {class_name}::foo() const {{
    return foo_;
}}

}} // namespace porytiles
"""

    impl_path.mkdir(parents=True, exist_ok=True)
    full_path.write_text(content)
    print(f"Created {full_path}")


def create_class_test(class_name, test_path, layer_path):
    """Create a C++ cpp file with a class test."""
    snake_class_name = camel_to_snake(class_name)
    cpp_filename = snake_class_name + '_test.cpp'
    full_path = test_path / cpp_filename

    if full_path.exists():
        print(f"file {full_path} already exists")
        return

    content = f"""#include "gtest/gtest.h"

#include "porytiles/{layer_path}/{snake_class_name}.hpp"

using namespace porytiles;

TEST({class_name}Tests, FooShouldBeZero) {{
    {class_name} foo{{}};
    EXPECT_EQ(foo.foo(), 0);
}}
"""

    test_path.mkdir(parents=True, exist_ok=True)
    full_path.write_text(content)
    print(f"Created {full_path}")


def main():
    project_root = Path(__file__).resolve().parent.parent

    if not (project_root / '.porytiles-marker-file').exists():
        print("Error: Could not find .porytiles-marker-file at project root", file=sys.stderr)
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description="Create a new C++ class header file and optionally cpp/test files"
    )
    parser.add_argument("class_name", help="CamelCase class name")
    parser.add_argument("layer_path", help="Layer path (e.g., domain/model)")
    parser.add_argument("--header-only", action="store_true",
                        help="Create only the header file, skip cpp and test files")
    parser.add_argument("--no-test", action="store_true",
                        help="Skip creating the test file")

    args = parser.parse_args()

    class_name = args.class_name
    layer_path = args.layer_path
    header_path = project_root / "Porytiles" / "include" / "porytiles" / layer_path
    impl_path = project_root / "Porytiles" / "lib" / layer_path
    test_path = project_root / "Porytiles" / "tests" / "unit" / layer_path

    if not class_name or not class_name[0].isupper():
        print("Error: Class name should start with an uppercase letter", file=sys.stderr)
        sys.exit(1)

    try:
        create_class_header(class_name, header_path)
    except Exception as e:
        print(f"Error creating header file: {e}", file=sys.stderr)
        sys.exit(1)

    if not args.header_only:
        try:
            create_class_impl(class_name, impl_path, layer_path)
        except Exception as e:
            print(f"Error creating cpp file: {e}", file=sys.stderr)
            sys.exit(1)

    if not args.header_only and not args.no_test:
        try:
            create_class_test(class_name, test_path, layer_path)
        except Exception as e:
            print(f"Error creating test file: {e}", file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()

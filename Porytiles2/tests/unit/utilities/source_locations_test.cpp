#include "gtest/gtest.h"

#include <source_location>

#include "porytiles2/utilities/source_locations.hpp"

using namespace porytiles2;

class SourceLocationsTest : public ::testing::Test {};

// Test GCC-style function signatures
TEST_F(SourceLocationsTest, GccStyleSimpleFunction)
{
    const std::string gcc_signature =
        "std::size_t porytiles2::LazyLayeredConfig::num_tiles_primary(const std::string&) const";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "num_tiles_primary");
}

TEST_F(SourceLocationsTest, GccStyleVoidFunction)
{
    const std::string gcc_signature = "void porytiles2::MyClass::do_something(int, double)";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "do_something");
}

TEST_F(SourceLocationsTest, GccStyleTemplateReturnType)
{
    const std::string gcc_signature = "std::vector<int> porytiles2::MyClass::get_values() const";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "get_values");
}

TEST_F(SourceLocationsTest, GccStylePointerReturnType)
{
    const std::string gcc_signature = "int* porytiles2::MyClass::get_pointer()";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "get_pointer");
}

TEST_F(SourceLocationsTest, GccStyleReferenceReturnType)
{
    const std::string gcc_signature = "const std::string& porytiles2::MyClass::get_name() const";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "get_name");
}

TEST_F(SourceLocationsTest, GccStyleNoNamespace)
{
    const std::string gcc_signature = "int MyClass::compute(int)";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "compute");
}

TEST_F(SourceLocationsTest, GccStyleFreeFunction)
{
    const std::string gcc_signature = "int porytiles2::compute_value(int, int)";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "compute_value");
}

// Test Clang-style function signatures
TEST_F(SourceLocationsTest, ClangStyleSimpleFunction)
{
    const std::string clang_signature = "porytiles2::LazyLayeredConfig::num_tiles_primary";
    EXPECT_EQ(extract_simple_function_name(clang_signature), "num_tiles_primary");
}

TEST_F(SourceLocationsTest, ClangStyleWithoutNamespace)
{
    const std::string clang_signature = "MyClass::do_something";
    EXPECT_EQ(extract_simple_function_name(clang_signature), "do_something");
}

TEST_F(SourceLocationsTest, ClangStyleFreeFunction)
{
    const std::string clang_signature = "porytiles2::compute_value";
    EXPECT_EQ(extract_simple_function_name(clang_signature), "compute_value");
}

TEST_F(SourceLocationsTest, ClangStyleNestedNamespace)
{
    const std::string clang_signature = "porytiles2::sub::MyClass::function_name";
    EXPECT_EQ(extract_simple_function_name(clang_signature), "function_name");
}

// Test edge cases
TEST_F(SourceLocationsTest, EmptyString)
{
    EXPECT_EQ(extract_simple_function_name(""), "");
}

TEST_F(SourceLocationsTest, SimpleNameOnly)
{
    const std::string simple_name = "my_function";
    EXPECT_EQ(extract_simple_function_name(simple_name), "my_function");
}

TEST_F(SourceLocationsTest, TrailingWhitespace)
{
    const std::string signature = "porytiles2::MyClass::my_function  ";
    EXPECT_EQ(extract_simple_function_name(signature), "my_function");
}

TEST_F(SourceLocationsTest, GccStyleComplexTemplateReturn)
{
    const std::string gcc_signature = "std::unique_ptr<std::vector<int>> porytiles2::MyClass::get_complex_type()";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "get_complex_type");
}

TEST_F(SourceLocationsTest, GccStyleLongQualifiedName)
{
    const std::string gcc_signature =
        "int namespace1::namespace2::namespace3::ClassName::method_name(int, double) const";
    EXPECT_EQ(extract_simple_function_name(gcc_signature), "method_name");
}

// Helper function to test actual source_location output
namespace {
std::string get_current_function_name(const std::source_location location = std::source_location::current())
{
    return extract_simple_function_name(location.function_name());
}
} // namespace

// Test with actual std::source_location output from the current compiler
TEST_F(SourceLocationsTest, RealSourceLocationOutput)
{
    const std::string function_name = get_current_function_name();
    // Google Test creates a TestBody() method for each test
    EXPECT_EQ(function_name, "TestBody");
}

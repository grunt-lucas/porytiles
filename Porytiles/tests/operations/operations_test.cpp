#include <gtest/gtest.h>

#include <memory>
#include <tuple>
#include <vector>

#include <gsl/pointers>

#include <porytiles/diagnostics/diagnostic_engine.hpp>
#include <porytiles/operations/operation.hpp>

using namespace porytiles;

TEST(OperationsTest, Foo) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};
    // Operation operation{&engine};
}

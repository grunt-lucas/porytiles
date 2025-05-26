#include <gtest/gtest.h>

#include <memory>
#include <tuple>
#include <vector>

#include <gsl/pointers>

#include <../../include/porytiles/orchestration/operation.hpp>
#include <porytiles/diagnostics/diagnostic_engine.hpp>

using namespace porytiles;

TEST(OperationsTest, Foo) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};
    // Operation operation{&engine};
}

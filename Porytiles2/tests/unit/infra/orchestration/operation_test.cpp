#include "gtest/gtest.h"

#include <expected>
#include <memory>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/infra/orchestration/operation.hpp"
#include "porytiles2/templates/result.hpp"

using namespace porytiles2;

class TestOperation final : public Operation {
  public:
    explicit TestOperation(DiagEngine *engine) : Operation{engine}, multiplier_{1} {}

    [[nodiscard]] std::vector<ArtifactDeclaration> declare_inputs() const override {
        std::vector inputs = {
            ArtifactDeclaration{"num1", typeid(int)},
            ArtifactDeclaration{"num2", typeid(int)},
        };
        return inputs;
    }

    /// @brief Declares the artifacts this operation will produce.
    [[nodiscard]] std::vector<ArtifactDeclaration> declare_outputs() const override {
        std::vector outputs = {ArtifactDeclaration{"sum", typeid(int)}};
        return outputs;
    }

    [[nodiscard]] Result<ArtifactBundle> execute(const ArtifactBundle &inputs) override {
        const auto num1 = inputs.get<int>("num1").value();
        const auto num2 = inputs.get<int>("num2").value();
        int sum = (num1 + num2) * multiplier_;
        ArtifactBundle outputs{};
        outputs.put("sum", sum);
        return outputs;
    }

    void set_multiplier(const int value) {
        multiplier_ = value;
    }

  private:
    int multiplier_;
};

TEST(OperationTests, BasicOperationFunctionsShouldWork) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};

    ArtifactBundle inputs{};
    inputs.put("num1", 10);
    inputs.put("num2", 5);

    TestOperation operation{&engine};
    operation.set_multiplier(10);

    const auto result = operation.execute(inputs);
    ASSERT_TRUE(result.has_value());

    const auto &map = result.value();
    const auto sum = map.get<int>("sum");
    EXPECT_EQ(150, sum);
}

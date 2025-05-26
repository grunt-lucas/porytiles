#include <gtest/gtest.h>

#include <memory>
#include <tuple>
#include <vector>

#include <gsl/pointers>

#include <../../include/porytiles/orchestration/operation.hpp>
#include <porytiles/diagnostics/diagnostic_engine.hpp>

using namespace porytiles;

class TestOperation final : public Operation {
  public:
    explicit TestOperation(DiagEngine *engine) : Operation{engine}, multiplier_{1} {}

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareInputs() const override {
        std::vector inputs = {
            ArtifactMetadata{"num1", typeid(int)},
            ArtifactMetadata{"num2", typeid(int)},
        };
        return inputs;
    }

    /// @brief Declares the artifacts this operation will produce.
    [[nodiscard]] std::vector<ArtifactMetadata> DeclareOutputs() const override {
        std::vector outputs = {ArtifactMetadata{"sum", typeid(int)}};
        return outputs;
    }

    [[nodiscard]] Result<AnyMap, BinaryStatus> Execute(const AnyMap &inputs) const override {
        const auto num1 = inputs.Get<int>("num1").value();
        const auto num2 = inputs.Get<int>("num2").value();
        int sum = (num1 + num2) * multiplier_;
        AnyMap outputs{};
        outputs.Put("sum", sum);
        return Result<AnyMap, BinaryStatus>{outputs};
    }

    void set_multiplier(const int value) {
        multiplier_ = value;
    }

  private:
    int multiplier_;
};

TEST(OperationTest, BasicOperationFunctionsShouldWork) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};

    AnyMap inputs{};
    inputs.Put("num1", 10);
    inputs.Put("num2", 5);

    TestOperation operation{&engine};
    operation.set_multiplier(10);

    const auto result = operation.Execute(inputs);
    ASSERT_TRUE(result.HasSuccess());
    const auto &map = result.Get();
    const auto sum = map.Get<int>("sum");
    ASSERT_EQ(150, sum);
}

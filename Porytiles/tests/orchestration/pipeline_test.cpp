#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include <gsl/pointers>

#include <porytiles/diagnostics/diagnostic_engine.hpp>
#include <porytiles/orchestration/operation.hpp>
#include <porytiles/orchestration/pipeline.hpp>

using namespace porytiles;

class NumSupplierOperation final : public Operation {
  public:
    explicit NumSupplierOperation(DiagEngine *engine, std::string key, const int value)
        : Operation{engine}, key_{std::move(key)}, value_{value} {}

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareInputs() const override {
        return {};
    }

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareOutputs() const override {
        return {ArtifactMetadata{key_, typeid(int)}};
    }

    [[nodiscard]] std::expected<AnyMap, std::string> Execute(const AnyMap &inputs) const override {
        AnyMap result{};
        result.Put(key_, value_);
        return std::expected<AnyMap, std::string>{result};
    }

  private:
    std::string key_;
    int value_;
};

class SumOperation final : public Operation {
  public:
    explicit SumOperation(DiagEngine *engine, std::vector<int> nums) : Operation{engine}, nums_{std::move(nums)} {}

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareInputs() const override {
        std::vector<ArtifactMetadata> inputs{};
        inputs.reserve(nums_.size());
        for (int i = 0; i < nums_.size(); ++i) {
            inputs.emplace_back("num" + std::to_string(i), typeid(int));
        }
        return inputs;
    }

    /// @brief Declares the artifacts this operation will produce.
    [[nodiscard]] std::vector<ArtifactMetadata> DeclareOutputs() const override {
        return {ArtifactMetadata{"sum", typeid(int)}};
    }

    [[nodiscard]] std::expected<AnyMap, std::string> Execute(const AnyMap &inputs) const override {
        int sum = 0;
        for (int i = 0; i < nums_.size(); ++i) {
            sum += inputs.Get<int>("num" + std::to_string(i)).value();
        }
        AnyMap outputs{};
        outputs.Put("sum", sum);
        return std::expected<AnyMap, std::string>{outputs};
    }

  private:
    std::vector<int> nums_;
};

class NumConsumerOperation final : public Operation {
  public:
    explicit NumConsumerOperation(DiagEngine *engine, std::string key) : Operation{engine}, key_{std::move(key)} {}

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareInputs() const override {
        return {ArtifactMetadata{key_, typeid(int)}};
    }

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareOutputs() const override {
        return {};
    }

    [[nodiscard]] std::expected<AnyMap, std::string> Execute(const AnyMap &inputs) const override {
        AnyMap result{};
        result.Put("result", inputs.Get<int>(key_).value());
        return std::expected<AnyMap, std::string>{result};
    }

  private:
    std::string key_;
};

TEST(PipelineTests, BasicPipelineShouldExecuteInCorrectOrder) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};
    const std::shared_ptr<Operation> supplierOp0 = std::make_shared<NumSupplierOperation>(&engine, "num0", 10);
    supplierOp0->set_name("supplierOp0");
    const std::shared_ptr<Operation> supplierOp1 = std::make_shared<NumSupplierOperation>(&engine, "num1", 20);
    supplierOp1->set_name("supplierOp1");
    const std::shared_ptr<Operation> sumOp = std::make_shared<SumOperation>(&engine, std::vector{0, 1});
    sumOp->set_name("sumOp");

    const Pipeline sum{std::vector{supplierOp0, supplierOp1, sumOp}};
    const auto result = sum.Run();
    ASSERT_EQ(30, result.value().Get<int>("sum"));
}

#include "gtest/gtest.h"

#include <memory>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/infra/orchestration/operation.hpp"
#include "porytiles2/infra/orchestration/pipeline.hpp"

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

    [[nodiscard]] std::expected<AnyMap, std::string> Execute(const AnyMap &inputs) override {
        AnyMap result{};
        result.Put(key_, value_);
        return result;
    }

  private:
    std::string key_;
    int value_;
};

class SumOperation final : public Operation {
  public:
    explicit SumOperation(DiagEngine *engine, std::vector<std::string> in_keys, std::string out_key = "sum")
        : Operation{engine}, in_keys_{std::move(in_keys)}, out_key_{std::move(out_key)} {}

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareInputs() const override {
        std::vector<ArtifactMetadata> inputs{};
        inputs.reserve(in_keys_.size());
        for (const auto &key : in_keys_) {
            inputs.emplace_back(key, typeid(int));
        }
        return inputs;
    }

    /// @brief Declares the artifacts this operation will produce.
    [[nodiscard]] std::vector<ArtifactMetadata> DeclareOutputs() const override {
        return {ArtifactMetadata{out_key_, typeid(int)}};
    }

    [[nodiscard]] std::expected<AnyMap, std::string> Execute(const AnyMap &inputs) override {
        int sum = 0;
        for (const auto &key : in_keys_) {
            sum += inputs.Get<int>(key).value();
        }
        AnyMap outputs{};
        outputs.Put(out_key_, sum);
        return outputs;
    }

  private:
    std::vector<std::string> in_keys_;
    std::string out_key_;
};

class NumConsumerOperation final : public Operation {
  public:
    explicit NumConsumerOperation(DiagEngine *engine, std::string key)
        : Operation{engine}, key_{std::move(key)}, consumed_{0} {}

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareInputs() const override {
        return {ArtifactMetadata{key_, typeid(int)}};
    }

    [[nodiscard]] std::vector<ArtifactMetadata> DeclareOutputs() const override {
        return {};
    }

    [[nodiscard]] std::expected<AnyMap, std::string> Execute(const AnyMap &inputs) override {
        consumed_ = inputs.Get<int>(key_).value();
        return {};
    }

    [[nodiscard]] int consumed() const {
        return consumed_;
    }

  private:
    std::string key_;
    int consumed_;
};

TEST(PipelineTests, BasicPipelineShouldExecuteInCorrectOrder) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};

    std::vector<std::shared_ptr<Operation>> ops{};
    ops.push_back(std::make_shared<NumSupplierOperation>(&engine, "num0", 10));
    ops.push_back(std::make_shared<NumSupplierOperation>(&engine, "num1", 20));
    ops.push_back(std::make_shared<SumOperation>(&engine, std::vector{std::string{"num0"}, std::string{"num1"}}));
    const auto consumerOp = std::make_shared<NumConsumerOperation>(&engine, "sum");
    ops.push_back(consumerOp);

    const Pipeline pipeline{ops};
    const auto result = pipeline.Run();
    ASSERT_EQ(30, std::dynamic_pointer_cast<NumConsumerOperation>(consumerOp)->consumed());
}

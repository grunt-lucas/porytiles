#include "gtest/gtest.h"

#include <memory>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/infra/orchestration/operation.hpp"
#include "porytiles2/infra/orchestration/pipeline.hpp"
#include "porytiles2/templates/result.hpp"

using namespace porytiles2;

class NumSupplierOperation final : public Operation {
public:
  explicit NumSupplierOperation(DiagEngine *engine, std::string key, const int value)
      : Operation{engine}, key_{std::move(key)}, value_{value} {}

  [[nodiscard]] std::vector<ArtifactDeclaration> declare_inputs() const override { return {}; }

  [[nodiscard]] std::vector<ArtifactDeclaration> declare_outputs() const override {
    return {ArtifactDeclaration{key_, typeid(int)}};
  }

  [[nodiscard]] Result<ArtifactBundle> execute(const ArtifactBundle &inputs) override {
    ArtifactBundle result{};
    result.put(key_, value_);
    return result;
  }

private:
  std::string key_;
  int value_;
};

class SumOperation final : public Operation {
public:
  explicit SumOperation(DiagEngine *engine, std::vector<std::string> in_keys,
                        std::string out_key = "sum")
      : Operation{engine}, in_keys_{std::move(in_keys)}, out_key_{std::move(out_key)} {}

  [[nodiscard]] std::vector<ArtifactDeclaration> declare_inputs() const override {
    std::vector<ArtifactDeclaration> inputs{};
    inputs.reserve(in_keys_.size());
    for (const auto &key : in_keys_) {
      inputs.emplace_back(key, typeid(int));
    }
    return inputs;
  }

  [[nodiscard]] std::vector<ArtifactDeclaration> declare_outputs() const override {
    return {ArtifactDeclaration{out_key_, typeid(int)}};
  }

  [[nodiscard]] Result<ArtifactBundle> execute(const ArtifactBundle &inputs) override {
    int sum = 0;
    for (const auto &key : in_keys_) {
      sum += inputs.get<int>(key).value();
    }
    ArtifactBundle outputs{};
    outputs.put(out_key_, sum);
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

  [[nodiscard]] std::vector<ArtifactDeclaration> declare_inputs() const override {
    return {ArtifactDeclaration{key_, typeid(int)}};
  }

  [[nodiscard]] std::vector<ArtifactDeclaration> declare_outputs() const override { return {}; }

  [[nodiscard]] Result<ArtifactBundle> execute(const ArtifactBundle &inputs) override {
    consumed_ = inputs.get<int>(key_).value();
    return {};
  }

  [[nodiscard]] int consumed() const { return consumed_; }

private:
  std::string key_;
  int consumed_;
};

TEST(PipelineTests, BasicPipelineShouldExecuteInCorrectOrder) {
  DiagEngine engine{std::make_unique<IgnoreConsumer>()};

  std::vector<std::shared_ptr<Operation>> ops{};
  ops.push_back(std::make_shared<NumSupplierOperation>(&engine, "num0", 10));
  ops.push_back(std::make_shared<NumSupplierOperation>(&engine, "num1", 20));
  ops.push_back(std::make_shared<SumOperation>(
      &engine, std::vector{std::string{"num0"}, std::string{"num1"}}));
  const auto consumerOp = std::make_shared<NumConsumerOperation>(&engine, "sum");
  ops.push_back(consumerOp);

  Pipeline pipeline{ops};
  const auto result = pipeline.run();
  ASSERT_EQ(30, consumerOp->consumed());
}

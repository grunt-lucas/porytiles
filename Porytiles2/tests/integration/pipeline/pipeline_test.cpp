#include "gtest/gtest.h"

#include <memory>
#include <vector>

#include "gsl/pointers"

#include "porytiles2/domain/orchestration/operand_bundle.hpp"
#include "porytiles2/domain/orchestration/operand_declaration.hpp"
#include "porytiles2/domain/orchestration/operation.hpp"
#include "porytiles2/domain/orchestration/pipeline.hpp"
#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

using namespace porytiles2;

class NumSupplierOperation final : public Operation {
  public:
    explicit NumSupplierOperation(std::string key, const int value) : key_{std::move(key)}, value_{value} {}

    [[nodiscard]] std::vector<OperandDeclaration> declare_inputs() const override
    {
        return {};
    }

    [[nodiscard]] std::vector<OperandDeclaration> declare_outputs() const override
    {
        return {OperandDeclaration{key_, typeid(int)}};
    }

    [[nodiscard]] std::string key() const
    {
        return key_;
    }

  protected:
    [[nodiscard]] ChainableResult<OperandBundle> execute(const OperandBundle &inputs) override
    {
        OperandBundle result{};
        result.put(key_, value_);
        return result;
    }

  private:
    std::string key_;
    int value_;
};

class SumOperation final : public Operation {
  public:
    explicit SumOperation(std::vector<std::string> in_keys, std::string out_key = "sum")
        : in_keys_{std::move(in_keys)}, out_key_{std::move(out_key)}
    {
    }

    [[nodiscard]] std::vector<OperandDeclaration> declare_inputs() const override
    {
        std::vector<OperandDeclaration> inputs{};
        inputs.reserve(in_keys_.size());
        for (const auto &key : in_keys_) {
            inputs.emplace_back(key, typeid(int));
        }
        return inputs;
    }

    [[nodiscard]] std::vector<OperandDeclaration> declare_outputs() const override
    {
        return {OperandDeclaration{out_key_, typeid(int)}};
    }

  protected:
    [[nodiscard]] ChainableResult<OperandBundle> execute(const OperandBundle &inputs) override
    {
        int sum = 0;
        for (const auto &key : in_keys_) {
            sum += inputs.get_unwrapped<int>(key).value();
        }
        OperandBundle outputs{};
        outputs.put(out_key_, sum);
        return outputs;
    }

  private:
    std::vector<std::string> in_keys_;
    std::string out_key_;
};

class NumConsumerOperation final : public Operation {
  public:
    explicit NumConsumerOperation(std::string key) : key_{std::move(key)}, consumed_{0} {}

    [[nodiscard]] std::vector<OperandDeclaration> declare_inputs() const override
    {
        return {OperandDeclaration{key_, typeid(int)}};
    }

    [[nodiscard]] std::vector<OperandDeclaration> declare_outputs() const override
    {
        return {};
    }

    [[nodiscard]] int consumed() const
    {
        return consumed_;
    }

  protected:
    [[nodiscard]] ChainableResult<OperandBundle> execute(const OperandBundle &inputs) override
    {
        consumed_ = inputs.get_unwrapped<int>(key_).value();
        return OperandBundle{};
    }

  private:
    std::string key_;
    int consumed_;
};

TEST(PipelineTests, BasicPipelineShouldExecuteInCorrectOrder)
{
    std::vector<Operation *> ops{};
    NumSupplierOperation num0_op{"num0", 10};
    NumSupplierOperation num1_op{"num1", 20};
    SumOperation sum_op{std::vector{num0_op.key(), num1_op.key()}};
    NumConsumerOperation consumer_op{"sum"};
    ops.push_back(&num0_op);
    ops.push_back(&num1_op);
    ops.push_back(&sum_op);
    ops.push_back(&consumer_op);

    Pipeline pipeline{ops};
    const auto result = pipeline.run();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(30, consumer_op.consumed());
}

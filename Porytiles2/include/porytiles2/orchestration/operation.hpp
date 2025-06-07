#pragma once

#include <expected>
#include <typeindex>

#include <gsl/pointers>

#include "../diagnostics/diagnostic_engine.hpp"
#include "../templates/any_map.hpp"

namespace porytiles {

class ArtifactMetadata {
  public:
    ArtifactMetadata(std::string key, const std::type_index type)
        : key_{std::move(key)}, expected_type_{type}, desc_{key} {}

    [[nodiscard]] const std::string &key() const {
        return key_;
    }

    [[nodiscard]] const std::type_index &expected_type() const {
        return expected_type_;
    }

    [[nodiscard]] const std::string &description() const {
        return desc_;
    }

    void set_description(const std::string &desc) {
        desc_ = desc;
    }

  private:
    std::string key_;
    std::type_index expected_type_;
    std::string desc_;
};

class Operation {
  public:
    virtual ~Operation() = default;

    explicit Operation(const gsl::not_null<DiagEngine *> diag) : diag_{diag} {}

    /// @brief Declares the input artifacts required by this operation.
    [[nodiscard]] virtual std::vector<ArtifactMetadata> DeclareInputs() const = 0;

    /// @brief Declares the artifacts this operation will produce.
    [[nodiscard]] virtual std::vector<ArtifactMetadata> DeclareOutputs() const = 0;

    [[nodiscard]] virtual std::expected<AnyMap, std::string> Execute(const AnyMap &inputs) = 0;

    [[nodiscard]] const DiagEngine &diag() const {
        return *diag_;
    }

    [[nodiscard]] const std::string &name() const {
        return name_;
    }

    void set_name(const std::string &name) {
        name_ = name;
    }

  private:
    DiagEngine *diag_;
    std::string name_;
};

} // namespace porytiles

#pragma once

#include <expected>
#include <typeindex>

#include "gsl/pointers"

#include "porytiles2/infra/diagnostics/diagnostic_engine.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief ArtifactDeclaration provides a POD-like class for Operation to declare input and output
 * artifact info.
 */
class ArtifactDeclaration {
public:
  ArtifactDeclaration(std::string key, const std::type_index type)
      : key_{std::move(key)}, expected_type_{type}, desc_{key} {}

  [[nodiscard]] const std::string &key() const { return key_; }

  [[nodiscard]] const std::type_index &expected_type() const { return expected_type_; }

  [[nodiscard]] const std::string &description() const { return desc_; }

  void set_description(const std::string &desc) { desc_ = desc; }

private:
  std::string key_;
  std::type_index expected_type_;
  std::string desc_;
};

class ArtifactBundle {
public:
  ArtifactBundle() = default;

  // -- Range-for support --
  using iterator = std::unordered_map<std::string, std::any>::iterator;
  using const_iterator = std::unordered_map<std::string, std::any>::const_iterator;

  iterator begin() noexcept { return config_.begin(); }
  iterator end() noexcept { return config_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return config_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return config_.end(); }
  [[nodiscard]] const_iterator cbegin() const noexcept { return config_.cbegin(); }
  [[nodiscard]] const_iterator cend() const noexcept { return config_.cend(); }

  template <typename T> [[nodiscard]] std::optional<T> get(const std::string &key) const {
    if (!contains(key)) {
      panic("Key not found: " + key);
    }
    try {
      return std::optional{std::any_cast<T>(config_.at(key))};
    } catch (const std::bad_any_cast &) {
      panic("Invalid type requested for key: " + key);
    }
  }

    [[nodiscard]] std::optional<std::any> try_get_any(const std::string &key) const {
    if (!contains(key)) {
      return std::nullopt;
    }
    return std::optional{config_.at(key)};
  }

  [[nodiscard]] std::any get_as_any(const std::string &key) const {
    if (!contains(key)) {
      panic("Key not found: " + key);
    }
    return config_.at(key);
  }

  void put(const std::string &key, const std::any &value) { config_.insert_or_assign(key, value); }

  [[nodiscard]] bool contains(const std::string &key) const { return config_.contains(key); }

  [[nodiscard]] std::optional<std::type_index> get_type(const std::string &key) const {
    if (!contains(key)) {
      return std::nullopt;
    }
    return config_.at(key).type();
  }

private:
  std::unordered_map<std::string, std::any> config_;
};

class Operation {
public:
  virtual ~Operation() = default;

  explicit Operation(const gsl::not_null<DiagEngine *> diag) : diag_{diag} {}

  /// @brief Declares the input artifacts required by this operation.
  [[nodiscard]] virtual std::vector<ArtifactDeclaration> declare_inputs() const = 0;

  /// @brief Declares the artifacts this operation will produce.
  [[nodiscard]] virtual std::vector<ArtifactDeclaration> declare_outputs() const = 0;

  [[nodiscard]] virtual Result<ArtifactBundle> execute(const ArtifactBundle &inputs) = 0;

  [[nodiscard]] const DiagEngine &diag() const { return *diag_; }

  [[nodiscard]] const std::string &name() const { return name_; }

  void set_name(const std::string &name) { name_ = name; }

private:
  DiagEngine *diag_;
  std::string name_;
};

} // namespace porytiles2

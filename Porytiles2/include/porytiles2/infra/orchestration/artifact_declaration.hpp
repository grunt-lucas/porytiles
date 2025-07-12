#pragma once

#include <expected>
#include <string>
#include <typeindex>

namespace porytiles2 {

/**
 * @brief ArtifactDeclaration provides a POD-like class for Operation to declare input and output
 * artifact info.
 */
class ArtifactDeclaration {
  public:
    ArtifactDeclaration(std::string key, const std::type_index type)
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

} // namespace porytiles2

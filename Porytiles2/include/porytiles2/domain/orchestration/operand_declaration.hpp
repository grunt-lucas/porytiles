#pragma once

#include <expected>
#include <string>
#include <typeindex>

namespace porytiles2 {

/**
 * @brief A specification for operands required or produced by orchestration operations.
 *
 * @details
 * OperandDeclaration provides a POD-like class that Operations use to declare input and output
 * operand metadata. Each declaration specifies a key name, expected type, and optional
 * human-readable description. This enables the orchestration framework to validate that
 * OperandDeclaration contents match Operation requirements at runtime.
 */
class OperandDeclaration {
  public:
    /**
     * @brief Constructs an operand declaration with the specified key and type.
     *
     * @details
     * The description is initialized to match the key by default, but can be
     * customized using set_description().
     *
     * @param key The unique identifier for this operand
     * @param type The expected C++ type represented as `std::type_index`
     */
    OperandDeclaration(std::string key, const std::type_index type)
        : key_{std::move(key)}, expected_type_{type}, desc_{key} {}

    /**
     * @brief Gets the operand's unique identifier.
     *
     * @return const reference to the key string
     */
    [[nodiscard]] const std::string &key() const {
        return key_;
    }

    /**
     * @brief Gets the expected type information for this operand.
     *
     * @return const reference to the std::type_index representing the expected type
     */
    [[nodiscard]] const std::type_index &expected_type() const {
        return expected_type_;
    }

    /**
     * @brief Gets the human-readable description of this operand.
     *
     * @return const reference to the description string
     */
    [[nodiscard]] const std::string &description() const {
        return desc_;
    }

    /**
     * @brief Sets a custom description for this operand.
     *
     * @param desc The new description text to associate with this operand
     */
    void set_description(const std::string &desc) {
        desc_ = desc;
    }

  private:
    std::string key_;
    std::type_index expected_type_;
    std::string desc_;
};

} // namespace porytiles2

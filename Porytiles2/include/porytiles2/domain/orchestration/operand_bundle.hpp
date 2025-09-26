#pragma once

#include <algorithm>
#include <any>
#include <optional>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "porytiles2/domain/orchestration/operand_declaration.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief A type-erased container for orchestration operands with runtime type checking.
 *
 * @details
 * OperandBundle provides a key-value store that can hold operands of any type using std::any.
 * It supports type-safe retrieval, validation against OperandDeclaration specifications,
 * and range-based iteration. This class is primarily used by the orchestration framework
 * to pass data between Operation instances while maintaining type safety at runtime.
 */
class OperandBundle {
  public:
    OperandBundle() = default;

    // -- Range-for support --
    using iterator = std::unordered_map<std::string, std::any>::iterator;
    using const_iterator = std::unordered_map<std::string, std::any>::const_iterator;
    iterator begin() noexcept;
    iterator end() noexcept;
    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
    [[nodiscard]] const_iterator cbegin() const noexcept;
    [[nodiscard]] const_iterator cend() const noexcept;
    // -- Range-for support --

    /**
     * @brief Retrieves an operand value as std::any.
     *
     * @details
     * Returns the operand associated with the given key wrapped in std::optional.
     * If the key does not exist, returns std::nullopt.
     *
     * @param key The string key identifying the operand
     * @return std::optional<std::any> containing the value if found, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::any> get(const std::string &key) const;

    /**
     * @brief Retrieves and casts an operand value to the specified type.
     *
     * @details
     * Performs type-safe retrieval and casting of an operand value. If the key exists
     * but the stored type does not match T, the function will panic. If the key does not
     * exist, returns std::nullopt.
     *
     * @tparam T The expected type of the operand
     * @param key The string key identifying the operand
     * @return std::optional<T> containing the cast value if found and type matches
     * @throws panic if key exists but type T does not match the stored type
     */
    template <typename T>
    [[nodiscard]] std::optional<T> get_unwrapped(const std::string &key) const
    {
        auto value = get(key);
        if (!value) {
            return std::nullopt;
        }
        try {
            return std::optional{std::any_cast<T>(value.value())};
        }
        catch (const std::bad_any_cast &) {
            panic("invalid type requested for key: " + key);
        }
    }

    /**
     * @brief Returns the number of operands stored in the bundle.
     *
     * @return The count of key-value pairs in the bundle
     */
    [[nodiscard]] std::size_t size() const;

    /**
     * @brief Stores an operand value with the given key.
     *
     * @details
     * Inserts or updates an operand in the bundle. If the key already exists,
     * the previous value is replaced.
     *
     * @param key The string key to associate with the value
     * @param value The operand value to store (type-erased as std::any)
     */
    void put(const std::string &key, const std::any &value);

    /**
     * @brief Checks if an operand with the given key exists.
     *
     * @param key The string key to check for existence
     * @return true if the key exists in the bundle, false otherwise
     */
    [[nodiscard]] bool contains(const std::string &key) const;

    /**
     * @brief Retrieves the runtime type information for an operand.
     *
     * @details
     * Returns the std::type_index of the value stored at the given key.
     * This is useful for runtime type checking and validation.
     *
     * @param key The string key identifying the operand
     * @return std::optional<std::type_index> containing the type if key exists, std::nullopt otherwise
     */
    [[nodiscard]] std::optional<std::type_index> type_index_of(const std::string &key) const;

    /**
     * @brief Validates that the bundle satisfies a set of operand declarations.
     *
     * @details
     * Checks that all required operands specified in the declarations are present
     * in the bundle and have the correct types. This is used by the orchestration
     * framework to validate Operation inputs and outputs.
     *
     * @param declarations Vector of OperandDeclaration objects specifying required operands
     * @return true if all declarations are satisfied, false otherwise
     */
    [[nodiscard]] bool satisfies_declarations(const std::vector<OperandDeclaration> &declarations) const;

  private:
    std::unordered_map<std::string, std::any> config_;
};

} // namespace porytiles2

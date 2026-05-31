#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include "porytiles/utilities/c_parser/source_position.hpp"
#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

/**
 * @brief Represents the value of a #define statement.
 *
 * @details
 * DefineValue captures the different types of values a #define can have:
 * - Integer value (evaluated from expression)
 * - String literal
 * - Empty (for flag-like defines with no value)
 */
using DefineValue = std::variant<std::int64_t, std::string, std::monostate>;

/**
 * @brief Represents a parsed #define preprocessor statement.
 *
 * @details
 * DefineStatement captures the name and value of a C preprocessor #define directive. For simple numeric defines like
 * `#define FOO 123` or `#define BAR (1 << 4)`, the value is evaluated and stored as an integer. For string defines like
 * `#define MSG "hello"`, the string value is stored. For flag-like defines with no value like `#define DEBUG`, the
 * value is std::monostate.
 *
 * The original source position is preserved for error reporting.
 *
 * Example defines this class can represent:
 * @code
 * #define FOO 123           // name="FOO", value=123
 * #define BAR 0xFF          // name="BAR", value=255
 * #define BAZ (1 << 4)      // name="BAZ", value=16
 * #define MSG "hello"       // name="MSG", value="hello"
 * #define DEBUG             // name="DEBUG", value=monostate
 * @endcode
 */
class DefineStatement {
  public:
    /**
     * @brief Constructs a DefineStatement with an integer value.
     *
     * @param name The macro name
     * @param value The integer value
     * @param position The source position of the #define
     */
    DefineStatement(std::string name, std::int64_t value, SourcePosition position)
        : name_{std::move(name)}, value_{value}, position_{position}
    {
    }

    /**
     * @brief Constructs a DefineStatement with a string value.
     *
     * @param name The macro name
     * @param value The string value
     * @param position The source position of the #define
     */
    DefineStatement(std::string name, std::string value, SourcePosition position)
        : name_{std::move(name)}, value_{std::move(value)}, position_{position}
    {
    }

    /**
     * @brief Constructs a DefineStatement with no value (flag-like define).
     *
     * @param name The macro name
     * @param position The source position of the #define
     */
    DefineStatement(std::string name, SourcePosition position)
        : name_{std::move(name)}, value_{std::monostate{}}, position_{position}
    {
    }

    /**
     * @brief Returns the macro name.
     *
     * @return A const reference to the name
     */
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /**
     * @brief Returns the value variant.
     *
     * @return A const reference to the DefineValue variant
     */
    [[nodiscard]] const DefineValue &value() const
    {
        return value_;
    }

    /**
     * @brief Returns the source position of the #define.
     *
     * @return A const reference to the source position
     */
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

    /**
     * @brief Checks if this define has an integer value.
     *
     * @return True if value holds an integer
     */
    [[nodiscard]] bool has_int_value() const
    {
        return std::holds_alternative<std::int64_t>(value_);
    }

    /**
     * @brief Checks if this define has a string value.
     *
     * @return True if value holds a string
     */
    [[nodiscard]] bool has_string_value() const
    {
        return std::holds_alternative<std::string>(value_);
    }

    /**
     * @brief Checks if this define has no value (flag-like).
     *
     * @return True if value holds monostate
     */
    [[nodiscard]] bool is_flag() const
    {
        return std::holds_alternative<std::monostate>(value_);
    }

    /**
     * @brief Returns the integer value.
     *
     * @pre has_int_value() == true
     * @return The integer value
     */
    [[nodiscard]] std::int64_t int_value() const
    {
        assert_or_panic(has_int_value(), "int_value() called on non-integer define");
        return std::get<std::int64_t>(value_);
    }

    /**
     * @brief Returns the string value.
     *
     * @pre has_string_value() == true
     * @return A const reference to the string value
     */
    [[nodiscard]] const std::string &string_value() const
    {
        assert_or_panic(has_string_value(), "string_value() called on non-string define");
        return std::get<std::string>(value_);
    }

  private:
    std::string name_;
    DefineValue value_;
    SourcePosition position_;
};

} // namespace porytiles

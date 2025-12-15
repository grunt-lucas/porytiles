#pragma once

#include <cstdint>
#include <string>

#include "porytiles2/utilities/c_parser/source_position.hpp"

namespace porytiles2 {

/**
 * @brief Represents a single member (constant) within a C enum declaration.
 *
 * @details
 * EnumMember captures the name and resolved value of an enum constant. Values may be explicitly assigned (e.g.,
 * `FOO = 10`) or implicitly assigned via counter (sequential from 0 or previous value + 1). The original source
 * position is preserved for error reporting.
 *
 * Example members this class can represent:
 * @code
 * enum {
 *     MB_NORMAL,          // name="MB_NORMAL", value=0, explicit=false
 *     MB_TALL_GRASS,      // name="MB_TALL_GRASS", value=1, explicit=false
 *     MB_INVALID = 0xFF,  // name="MB_INVALID", value=255, explicit=true
 * };
 * @endcode
 */
class EnumMember {
  public:
    /**
     * @brief Constructs an EnumMember with an implicit (counter-based) value.
     *
     * @param name The member name
     * @param value The resolved integer value
     * @param position The source position of the member
     */
    EnumMember(std::string name, std::int64_t value, SourcePosition position)
        : name_{std::move(name)}, value_{value}, has_explicit_value_{false}, position_{position}
    {
    }

    /**
     * @brief Constructs an EnumMember with explicit value flag.
     *
     * @param name The member name
     * @param value The resolved integer value
     * @param has_explicit_value True if value was explicitly assigned with `=`
     * @param position The source position of the member
     */
    EnumMember(std::string name, std::int64_t value, bool has_explicit_value, SourcePosition position)
        : name_{std::move(name)}, value_{value}, has_explicit_value_{has_explicit_value}, position_{position}
    {
    }

    /**
     * @brief Returns the member name.
     *
     * @return A const reference to the name
     */
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /**
     * @brief Returns the resolved integer value.
     *
     * @return The enum member value
     */
    [[nodiscard]] std::int64_t value() const
    {
        return value_;
    }

    /**
     * @brief Checks if this member had an explicit value assignment.
     *
     * @return True if the value was explicitly assigned with `=`
     */
    [[nodiscard]] bool has_explicit_value() const
    {
        return has_explicit_value_;
    }

    /**
     * @brief Returns the source position of the member.
     *
     * @return A const reference to the source position
     */
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

  private:
    std::string name_;
    std::int64_t value_;
    bool has_explicit_value_;
    SourcePosition position_;
};

} // namespace porytiles2

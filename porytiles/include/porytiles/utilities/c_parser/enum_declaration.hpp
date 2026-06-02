#pragma once

#include <optional>
#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/enum_member.hpp"
#include "porytiles/utilities/c_parser/source_position.hpp"

namespace porytiles {

/**
 * @brief Represents a parsed C enum declaration.
 *
 * @details
 * EnumDeclaration captures the complete structure of a C enum, including its optional name (anonymous enums have no
 * name) and all its members. The original source position of the `enum` keyword is preserved for error reporting.
 *
 * Example enums this class can represent:
 * @code
 * enum { MB_NORMAL, MB_TALL_GRASS };              // anonymous, name=nullopt
 * enum MetatileBehavior { MB_NORMAL, MB_TALL_GRASS };  // named
 * @endcode
 */
class EnumDeclaration {
  public:
    /**
     * @brief Constructs an anonymous enum declaration.
     *
     * @param members The enum members
     * @param position The source position of the `enum` keyword
     */
    EnumDeclaration(std::vector<EnumMember> members, SourcePosition position)
        : name_{std::nullopt}, members_{std::move(members)}, position_{position}
    {
    }

    /**
     * @brief Constructs a named enum declaration.
     *
     * @param name The enum name
     * @param members The enum members
     * @param position The source position of the `enum` keyword
     */
    EnumDeclaration(std::string name, std::vector<EnumMember> members, SourcePosition position)
        : name_{std::move(name)}, members_{std::move(members)}, position_{position}
    {
    }

    /**
     * @brief Checks if this enum has a name.
     *
     * @return True if the enum has a name (not anonymous)
     */
    [[nodiscard]] bool has_name() const
    {
        return name_.has_value();
    }

    /**
     * @brief Returns the enum name if present.
     *
     * @return A const reference to the optional name
     */
    [[nodiscard]] const std::optional<std::string> &name() const
    {
        return name_;
    }

    /**
     * @brief Returns the enum members.
     *
     * @return A const reference to the member vector
     */
    [[nodiscard]] const std::vector<EnumMember> &members() const
    {
        return members_;
    }

    /**
     * @brief Returns the source position of the `enum` keyword.
     *
     * @return A const reference to the source position
     */
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

  private:
    std::optional<std::string> name_;
    std::vector<EnumMember> members_;
    SourcePosition position_;
};

} // namespace porytiles

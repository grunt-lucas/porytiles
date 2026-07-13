#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/source_position.hpp"

namespace porytiles {

/// @brief One member declaration inside a C struct definition.
///
/// @details
/// Captures the simple declarator shape `[const] TYPE [*...] NAME [: WIDTH];`. @c type_name is the single type
/// identifier (a typedef name like @c u16, or the tag name for `struct TYPE` members). @c pointer_depth counts the
/// leading stars on the declarator (0 for a value member, 1 for `const u16 *metatileAttributes`, and so on). @c
/// is_const is true when the declaration carries a @c const qualifier. Bitfield widths are consumed but not recorded.
///
/// Members whose declarators fall outside this shape (pointer-to-array members like `const u16 (*palettes)[16]`,
/// array members, multiple declarators per statement) are skipped by the parser rather than represented here.
struct StructMemberDeclaration {
    std::string type_name;
    std::size_t pointer_depth{0};
    std::string member_name;
    bool is_const{false};
    SourcePosition position;
};

/// @brief A parsed C struct type definition.
///
/// @details
/// Captures definitions of the form:
/// @code
/// struct Tileset
/// {
///     /*0x00*/ u8 isCompressed:1;
///     /*0x01*/ bool8 isSecondary;
///     /*0x10*/ const u16 *metatileAttributes;
///     /*0x14*/ TilesetCB callback;
/// };
/// @endcode
///
/// Only named definitions with a member body are captured; forward declarations and anonymous typedef structs are
/// not. @c members holds the declarations the parser could pattern-match, in declaration order; unparseable members
/// are skipped tolerantly, never aborting the scan.
struct StructDefinition {
    std::string name;
    std::vector<StructMemberDeclaration> members;
    SourcePosition position;
};

} // namespace porytiles

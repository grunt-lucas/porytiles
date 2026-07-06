#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/source_position.hpp"
#include "porytiles/utilities/c_parser/token.hpp"

namespace porytiles {

/**
 * @brief One designated-initializer entry of an indexed array.
 *
 * @details
 * Represents a single `[index] = value` element. @c index_name is the text of the designator (typically an enum
 * member name such as `METATILE_ATTRIBUTE_BEHAVIOR`, or a numeric literal). @c value holds the evaluated value when the
 * value expression could be resolved from the parser's symbol table; it is absent when the expression referenced an
 * unknown symbol. @c value_tokens keeps the raw value expression so a caller can re-evaluate it against a richer symbol
 * table later.
 */
struct IndexedArrayEntry {
    std::string index_name;
    std::optional<std::int64_t> value;
    std::vector<Token> value_tokens;
    SourcePosition position;
};

/**
 * @brief A parsed C array declaration that uses designated initializers.
 *
 * @details
 * Captures declarations of the form:
 * @code
 * static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
 *     [METATILE_ATTRIBUTE_BEHAVIOR]      = 0x000001ff,
 *     [METATILE_ATTRIBUTE_TERRAIN]       = 0x00003e00,
 *     ...
 * };
 * @endcode
 *
 * The array's size expression is skipped; only the name and the designated entries are retained.
 */
struct IndexedArrayDeclaration {
    std::string name;
    std::vector<IndexedArrayEntry> entries;
    SourcePosition position;
};

} // namespace porytiles

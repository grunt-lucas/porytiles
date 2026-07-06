#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/define_statement.hpp"
#include "porytiles/utilities/c_parser/source_position.hpp"

namespace porytiles {

/**
 * @brief A construct the tolerant scanners recorded but could not fully evaluate.
 *
 * @details
 * Tolerant scans do not abort when a single construct fails to evaluate (for example a #define whose value references a
 * macro from an unparsed header). Instead the offender is captured here with enough context to report it, and scanning
 * continues. The name still counts as defined for later conditional decisions.
 */
struct SkippedConstruct {
    std::string name;
    SourcePosition position;
    std::string reason;

    bool operator==(const SkippedConstruct &) const = default;
};

/**
 * @brief The result of a tolerant #define scan.
 *
 * @details
 * @c defines holds every define whose value was resolved (or that carries no value). @c skipped holds the defines whose
 * value could not be evaluated; their names were still recorded as defined.
 */
struct TolerantDefineScan {
    std::vector<DefineStatement> defines;
    std::vector<SkippedConstruct> skipped;
};

/**
 * @brief One enum member from a tolerant enum scan.
 *
 * @details
 * @c value is absent when the member's value could not be determined: either its explicit expression was unevaluable,
 * or it is an implicit member whose running counter was poisoned by an earlier unevaluable explicit value.
 */
struct TolerantEnumMember {
    std::string name;
    std::optional<std::int64_t> value;
    SourcePosition position;
};

/**
 * @brief One enum declaration from a tolerant enum scan.
 */
struct TolerantEnum {
    std::optional<std::string> name;
    std::vector<TolerantEnumMember> members;
    SourcePosition position;
};

/**
 * @brief The result of a tolerant enum scan.
 *
 * @details
 * @c enums holds every enum that parsed structurally, each with per-member values that may be absent. @c skipped holds
 * enums that could not be parsed structurally (for example a malformed body).
 */
struct TolerantEnumScan {
    std::vector<TolerantEnum> enums;
    std::vector<SkippedConstruct> skipped;
};

} // namespace porytiles

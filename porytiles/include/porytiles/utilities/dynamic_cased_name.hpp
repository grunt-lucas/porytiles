#pragma once

#include <compare>
#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace porytiles {

/**
 * @brief A smart string wrapper that preserves word structure for lossless case format conversion.
 *
 * @details
 * Porytiles deals with tileset names, animation names, and identifiers that come in various case formats (PascalCase,
 * snake_case, smoosh_case, and C identifiers with mixed casing around underscores). Converting between formats using
 * simple string manipulation is lossy. Particularly, round-trips through snake_case lose information about the original
 * underscore positions vs PascalCase word boundaries.
 *
 * DynamicCasedName solves this by parsing the input into a two-level structure:
 * - Outer vector ("segments"): groups separated by underscores in C identifier format
 * - Inner vector ("words"): individual words within a segment, split by PascalCase boundaries
 * - All words stored fully lowercase
 *
 * This structure enables lossless conversion to any output format from any input format. Equality and ordering are
 * based on the canonical (all-lowercase, no-separator) form, so names that represent the same identifier in different
 * formats compare as equal.
 *
 * @invariant All words in @c segments_ are non-empty and fully lowercase.
 * @invariant @c canonical_ equals all words concatenated with no separators.
 */
class DynamicCasedName {
  public:
    DynamicCasedName() = default;

    /**
     * @brief Constructs a DynamicCasedName by auto-detecting the input format.
     *
     * @details
     * Auto-detection uses the following heuristic:
     * - Has underscores AND uppercase letters -> C identifier format
     * - Has underscores but no uppercase -> snake_case
     * - No underscores but has uppercase -> PascalCase
     * - No underscores and no uppercase -> smoosh_case
     *
     * @param input The name string to parse.
     */
    explicit DynamicCasedName(const std::string &input);

    /**
     * @brief Constructs from a snake_case input string.
     *
     * @details
     * Splits on underscores. Each token becomes a single-word segment. Empty tokens from leading, trailing, or
     * consecutive underscores are filtered out.
     *
     * @param input The snake_case string to parse.
     * @return A DynamicCasedName with the parsed structure.
     */
    [[nodiscard]] static DynamicCasedName from_snake_case(const std::string &input);

    /**
     * @brief Constructs from a PascalCase input string.
     *
     * @details
     * Splits on PascalCase word boundaries. All words go into a single segment. Acronyms like "XML" or "TV" are
     * handled using the same boundary logic as @c to_snake_case() in @c string_utils.hpp.
     *
     * @param input The PascalCase string to parse.
     * @return A DynamicCasedName with the parsed structure.
     */
    [[nodiscard]] static DynamicCasedName from_pascal_case(const std::string &input);

    /**
     * @brief Constructs from a C identifier format string (PascalCase segments joined by underscores).
     *
     * @details
     * First splits on underscores into tokens, then splits each token by PascalCase boundaries. Each underscore-
     * delimited token becomes one segment containing its PascalCase-split words.
     *
     * @param input The C identifier string to parse (e.g., "Water_Current_LandWatersEdge").
     * @return A DynamicCasedName with the parsed structure.
     */
    [[nodiscard]] static DynamicCasedName from_c_identifier(const std::string &input);

    /**
     * @brief Constructs from a flatcase input string (all lowercase, no separators).
     *
     * @details
     * The entire lowercased input becomes a single atomic word in a single segment. No word splitting is attempted
     * since flatcase provides no boundary information.
     *
     * @param input The flatcase string to parse.
     * @return A DynamicCasedName with the parsed structure.
     */
    [[nodiscard]] static DynamicCasedName from_flat_case(const std::string &input);

    /**
     * @brief Outputs all words flattened and joined with underscores.
     *
     * @return The name in snake_case format.
     */
    [[nodiscard]] std::string to_snake_case() const;

    /**
     * @brief Outputs all words flattened and joined in PascalCase (each word capitalized, no separators).
     *
     * @return The name in PascalCase format.
     */
    [[nodiscard]] std::string to_pascal_case() const;

    /**
     * @brief Outputs PascalCase within each segment, with segments joined by underscores.
     *
     * @return The name in C identifier format.
     */
    [[nodiscard]] std::string to_c_identifier() const;

    /**
     * @brief Outputs all words joined with no separators, all lowercase.
     *
     * @return The name in flatcase format.
     */
    [[nodiscard]] std::string to_flat_case() const;

    /**
     * @brief Returns the canonical form (all words lowercase, no separators).
     *
     * @details
     * The canonical form is used for equality, ordering, and hashing. Two DynamicCasedName objects that represent the
     * same logical name in different formats will have identical canonical forms.
     *
     * @return A const reference to the canonical string.
     */
    [[nodiscard]] const std::string &canonical() const
    {
        return canonical_;
    }

    /**
     * @brief Checks if this name is empty (has no segments/words).
     *
     * @return True if the name contains no segments.
     */
    [[nodiscard]] bool empty() const
    {
        return segments_.empty();
    }

    /**
     * @brief Returns the internal two-level segment/word structure.
     *
     * @return A const reference to the segments vector.
     */
    [[nodiscard]] const std::vector<std::vector<std::string>> &segments() const
    {
        return segments_;
    }

    /**
     * @brief Equality comparison based on canonical form.
     *
     * @param other The other DynamicCasedName to compare with.
     * @return True if both names have the same canonical form.
     */
    [[nodiscard]] bool operator==(const DynamicCasedName &other) const
    {
        return canonical_ == other.canonical_;
    }

    /**
     * @brief Three-way comparison based on canonical form.
     *
     * @param other The other DynamicCasedName to compare with.
     * @return The ordering relationship between the canonical forms.
     */
    [[nodiscard]] auto operator<=>(const DynamicCasedName &other) const
    {
        return canonical_ <=> other.canonical_;
    }

  private:
    std::vector<std::vector<std::string>> segments_;
    std::string canonical_;

    explicit DynamicCasedName(std::vector<std::vector<std::string>> segments);
    void compute_canonical();
};

/**
 * @brief Converts a DynamicCasedName to its snake_case string representation.
 *
 * @param value The DynamicCasedName to convert.
 * @return The snake_case representation of the name.
 */
[[nodiscard]] std::string to_string(const DynamicCasedName &value);

/**
 * @brief Stream insertion operator for DynamicCasedName.
 *
 * @param os The output stream.
 * @param value The DynamicCasedName to output.
 * @return Reference to the output stream.
 */
inline std::ostream &operator<<(std::ostream &os, const DynamicCasedName &value)
{
    os << to_string(value);
    return os;
}

} // namespace porytiles

template <>
struct std::hash<porytiles::DynamicCasedName> {
    std::size_t operator()(const porytiles::DynamicCasedName &name) const noexcept
    {
        return std::hash<std::string>{}(name.canonical());
    }
};

/**
 * @brief std::formatter specialization for DynamicCasedName.
 *
 * @details
 * Enables DynamicCasedName to be used with std::format() and related formatting functions. Delegates to the
 * porytiles::to_string() overload for consistent string representation.
 */
template <>
struct std::formatter<porytiles::DynamicCasedName> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::DynamicCasedName &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};

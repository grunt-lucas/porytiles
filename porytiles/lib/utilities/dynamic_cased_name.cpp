#include "porytiles/utilities/dynamic_cased_name.hpp"

#include <cctype>
#include <string>
#include <vector>

namespace porytiles {
namespace {

/// @brief Splits a PascalCase or camelCase token into lowercase words.
///
/// @details
/// Mirrors the word-boundary logic from to_snake_case() in string_utils.hpp: insert a word break before an uppercase
/// character if the previous character was lowercase, OR if the next character is lowercase (to handle acronyms like
/// "XMLParser" -> ["xml", "parser"] and "TVTurnedOn" -> ["tv", "turned", "on"]).
///
/// @param token The token to split.
/// @return A vector of lowercase words.
[[nodiscard]] std::vector<std::string> split_pascal_words(const std::string &token)
{
    if (token.empty()) {
        return {};
    }

    std::vector<std::string> words;
    std::string current_word;

    for (std::size_t i = 0; i < token.size(); ++i) {
        const char c = token[i];

        if (std::isupper(static_cast<unsigned char>(c))) {
            // Insert a word break before this uppercase char if:
            // 1. We have accumulated characters already, AND
            // 2. Either the previous char was lowercase, OR the next char is lowercase (acronym boundary)
            if (!current_word.empty()) {
                const bool prev_is_lower = i > 0 && std::islower(static_cast<unsigned char>(token[i - 1]));
                const bool next_is_lower =
                    i + 1 < token.size() && std::islower(static_cast<unsigned char>(token[i + 1]));
                if (prev_is_lower || next_is_lower) {
                    words.push_back(current_word);
                    current_word.clear();
                }
            }
            current_word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        else {
            current_word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    if (!current_word.empty()) {
        words.push_back(current_word);
    }

    return words;
}

/// @brief Splits a string on underscores, filtering out empty tokens.
///
/// @param input The string to split.
/// @return A vector of non-empty tokens.
[[nodiscard]] std::vector<std::string> split_on_underscores(const std::string &input)
{
    std::vector<std::string> tokens;
    std::string current;

    for (const char c : input) {
        if (c == '_') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        else {
            current += c;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

/// @brief Capitalizes the first character of a string.
///
/// @param word The word to capitalize.
/// @return The word with its first character uppercased.
[[nodiscard]] std::string capitalize(const std::string &word)
{
    if (word.empty()) {
        return word;
    }
    std::string result = word;
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}

} // namespace

DynamicCasedName::DynamicCasedName(std::vector<std::vector<std::string>> segments) : segments_{std::move(segments)}
{
    compute_canonical();
}

void DynamicCasedName::compute_canonical()
{
    canonical_.clear();
    for (const auto &segment : segments_) {
        for (const auto &word : segment) {
            canonical_ += word;
        }
    }
}

DynamicCasedName::DynamicCasedName(const std::string &input)
{
    if (input.empty()) {
        return;
    }

    bool has_underscore = false;
    bool has_uppercase = false;

    for (const char c : input) {
        if (c == '_') {
            has_underscore = true;
        }
        if (std::isupper(static_cast<unsigned char>(c))) {
            has_uppercase = true;
        }
    }

    if (has_underscore && has_uppercase) {
        *this = from_c_identifier(input);
    }
    else if (has_underscore) {
        *this = from_snake_case(input);
    }
    else if (has_uppercase) {
        *this = from_pascal_case(input);
    }
    else {
        *this = from_flat_case(input);
    }
}

DynamicCasedName DynamicCasedName::from_snake_case(const std::string &input)
{
    if (input.empty()) {
        return DynamicCasedName{};
    }

    std::vector<std::string> tokens = split_on_underscores(input);
    std::vector<std::vector<std::string>> segments;
    segments.reserve(tokens.size());

    for (auto &token : tokens) {
        // Lowercase the token
        std::string lower;
        lower.reserve(token.size());
        for (const char c : token) {
            lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        segments.push_back({std::move(lower)});
    }

    return DynamicCasedName{std::move(segments)};
}

DynamicCasedName DynamicCasedName::from_pascal_case(const std::string &input)
{
    if (input.empty()) {
        return DynamicCasedName{};
    }

    std::vector<std::string> words = split_pascal_words(input);
    if (words.empty()) {
        return DynamicCasedName{};
    }

    return DynamicCasedName{std::vector<std::vector<std::string>>{std::move(words)}};
}

DynamicCasedName DynamicCasedName::from_c_identifier(const std::string &input)
{
    if (input.empty()) {
        return DynamicCasedName{};
    }

    std::vector<std::string> tokens = split_on_underscores(input);
    std::vector<std::vector<std::string>> segments;
    segments.reserve(tokens.size());

    for (const auto &token : tokens) {
        std::vector<std::string> words = split_pascal_words(token);
        if (!words.empty()) {
            segments.push_back(std::move(words));
        }
    }

    return DynamicCasedName{std::move(segments)};
}

DynamicCasedName DynamicCasedName::from_flat_case(const std::string &input)
{
    if (input.empty()) {
        return DynamicCasedName{};
    }

    std::string lower;
    lower.reserve(input.size());
    for (const char c : input) {
        lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return DynamicCasedName{std::vector<std::vector<std::string>>{{std::move(lower)}}};
}

std::string DynamicCasedName::to_snake_case() const
{
    std::string result;
    bool first = true;

    for (const auto &segment : segments_) {
        for (const auto &word : segment) {
            if (!first) {
                result += '_';
            }
            result += word;
            first = false;
        }
    }

    return result;
}

std::string DynamicCasedName::to_pascal_case() const
{
    std::string result;

    for (const auto &segment : segments_) {
        for (const auto &word : segment) {
            result += capitalize(word);
        }
    }

    return result;
}

std::string DynamicCasedName::to_c_identifier() const
{
    std::string result;
    bool first_segment = true;

    for (const auto &segment : segments_) {
        if (!first_segment) {
            result += '_';
        }
        for (const auto &word : segment) {
            result += capitalize(word);
        }
        first_segment = false;
    }

    return result;
}

std::string DynamicCasedName::to_flat_case() const
{
    return canonical_;
}

std::string to_string(const DynamicCasedName &value)
{
    return value.to_snake_case();
}

} // namespace porytiles

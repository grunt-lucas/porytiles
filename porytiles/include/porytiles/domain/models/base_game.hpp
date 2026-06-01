#pragma once

#include <format>
#include <optional>
#include <ostream>
#include <string>

#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

/**
 * @brief Identifies the target base game for a decompilation project.
 *
 * @details
 * Different base games use different binary formats for metatile attributes:
 * - pokeemerald / pokeruby / pokeemerald_expansion: 2-byte attributes (behavior + layer type)
 * - pokefirered: 4-byte attributes (behavior + terrain + encounter type + layer type + more)
 */
enum class BaseGame { pokeemerald, pokefirered, pokeruby, pokeemerald_expansion };

/**
 * @brief Converts a string to BaseGame.
 *
 * @param s The string to convert (e.g. "pokeemerald", "pokefirered", "pokeruby", "pokeemerald-expansion")
 * @return The corresponding BaseGame, or std::nullopt if the string is invalid
 */
[[nodiscard]] inline std::optional<BaseGame> base_game_from_str(const std::string &s)
{
    if (s == "pokeemerald") {
        return BaseGame::pokeemerald;
    }
    if (s == "pokefirered") {
        return BaseGame::pokefirered;
    }
    if (s == "pokeruby") {
        return BaseGame::pokeruby;
    }
    if (s == "pokeemerald-expansion") {
        return BaseGame::pokeemerald_expansion;
    }
    return std::nullopt;
}

/**
 * @brief Converts BaseGame to string representation.
 *
 * @param game The BaseGame to convert
 * @return String representation (e.g. "pokeemerald", "pokefirered", "pokeruby", "pokeemerald-expansion")
 */
[[nodiscard]] inline std::string to_string(BaseGame game)
{
    switch (game) {
    case BaseGame::pokeemerald:
        return "pokeemerald";
    case BaseGame::pokefirered:
        return "pokefirered";
    case BaseGame::pokeruby:
        return "pokeruby";
    case BaseGame::pokeemerald_expansion:
        return "pokeemerald-expansion";
    }
    panic("unhandled BaseGame value");
}

/**
 * @brief Stream insertion operator for BaseGame.
 *
 * @param os The output stream
 * @param game The BaseGame to output
 * @return The output stream
 */
inline std::ostream &operator<<(std::ostream &os, const BaseGame game)
{
    return os << to_string(game);
}

} // namespace porytiles

template <>
struct std::formatter<porytiles::BaseGame> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::BaseGame &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};

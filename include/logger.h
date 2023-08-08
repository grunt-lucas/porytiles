#ifndef PORYTILES_LOGGER_H
#define PORYTILES_LOGGER_H

#include <cstdio>
#include <stdexcept>
#include <string>

#define FMT_HEADER_ONLY
#include <fmt/color.h>

#include "program_name.h"
#include "ptcontext.h"
#include "types.h"

namespace porytiles {

template <typename... T>
void pt_logln(const PtContext &ctx, std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  if (ctx.verbose) {
    fmt::println(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
  }
}

template <typename... T> void pt_log(const PtContext &ctx, std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  if (ctx.verbose) {
    fmt::print(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
  }
}

template <typename... T> void pt_println(std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_print(std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  fmt::print(stream, "{}", fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_msg(std::FILE *stream, fmt::format_string<T...> fmt, T &&...args)
{
  fmt::print(stream, "{}: {}", PROGRAM_NAME, fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_err(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("error:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)),
               fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_err_rgbatile(const RGBATile &tile, fmt::format_string<T...> fmt, T &&...args)
{
  std::string prefix = "";
  if (tile.type == TileType::FREESTANDING) {
    prefix = "tile:" + std::to_string(tile.tileIndex);
  }
  else if (tile.type == TileType::LAYERED) {
    prefix = layerString(tile.layer) + ":" + std::to_string(tile.metatileIndex) + ":" + subtileString(tile.subtile);
  }
  else if (tile.type == TileType::ANIM) {
    prefix = tile.anim + ":" + tile.frame + ":" + std::to_string(tile.tileIndex);
  }
  else {
    throw std::runtime_error{"pt_err_tileprefix unknown TileType"};
  }
  fmt::println(stderr, "{} {} {}", fmt::styled(prefix + ":", fmt::emphasis::bold),
               fmt::styled("error:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)),
               fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_fatal_err(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("fatal error:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::red)),
               fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_warn(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("warning:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::magenta)),
               fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_note(fmt::format_string<T...> fmt, T &&...args)
{
  fmt::println(stderr, "{}: {} {}", PROGRAM_NAME,
               fmt::styled("note:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::cyan)),
               fmt::format(fmt, std::forward<T>(args)...));
}

template <typename... T> void pt_note_rgbatile(const RGBATile &tile, fmt::format_string<T...> fmt, T &&...args)
{
  std::string prefix = "";
  if (tile.type == TileType::FREESTANDING) {
    prefix = "tile:" + std::to_string(tile.tileIndex);
  }
  else if (tile.type == TileType::LAYERED) {
    prefix = layerString(tile.layer) + ":" + std::to_string(tile.metatileIndex) + ":" + subtileString(tile.subtile);
  }
  else if (tile.type == TileType::ANIM) {
    prefix = tile.anim + ":" + tile.frame + ":" + std::to_string(tile.tileIndex);
  }
  else {
    throw std::runtime_error{"pt_err_tileprefix unknown TileType"};
  }
  fmt::println(stderr, "{} {} {}", fmt::styled(prefix + ":", fmt::emphasis::bold),
               fmt::styled("note:", fmt::emphasis::bold | fmt::fg(fmt::terminal_color::cyan)),
               fmt::format(fmt, std::forward<T>(args)...));
}

} // namespace porytiles

#endif // PORYTILES_LOGGER_H

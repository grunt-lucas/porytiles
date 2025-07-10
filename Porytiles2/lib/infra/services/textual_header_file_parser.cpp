#include "porytiles2/infra/services/textual_header_file_parser.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "fmt/format.h"

#include "porytiles2/domain/services/header_file_parser.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Read all lines from a file.
 *
 * @param file_path The path to the file to read
 * @return Result<std::vector<std::string>> containing the file lines or error details
 */
Result<std::vector<std::string>> read_file_lines(const std::filesystem::path &file_path) {
  try {
    std::ifstream file{file_path};
    if (!file.is_open()) {
      return std::unexpected{fmt::format("Cannot open file for reading: {}", file_path.string())};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
      lines.push_back(line);
    }

    return lines;
  } catch (const std::exception &e) {
    return std::unexpected{
        fmt::format("Exception reading file {}: {}", file_path.string(), e.what())};
  }
}

/**
 * @brief Trim whitespace from a string.
 *
 * @param str The string to trim
 * @return The trimmed string
 */
std::string trim_string(const std::string &str) {
  auto start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos) {
    return "";
  }

  auto end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

/**
 * @brief Check if a line contains recognizable C header patterns.
 *
 * @param trimmed_line The trimmed line to check for C header patterns
 * @return True if the line contains C header patterns, false otherwise
 */
bool has_c_header_patterns(const std::string &trimmed_line) {
  return trimmed_line.starts_with("#include") || trimmed_line.starts_with("#ifndef") ||
         trimmed_line.starts_with("#define") || trimmed_line.starts_with("const ") ||
         trimmed_line.starts_with("extern ") || trimmed_line.starts_with("struct ") ||
         trimmed_line.find("INCBIN") != std::string::npos;
}

} // anonymous namespace

Result<std::vector<std::string>>
TextualHeaderFileParser::parse_header_file(const std::filesystem::path &file_path) {
  return read_file_lines(file_path);
}

Result<bool> TextualHeaderFileParser::contains_declaration(const std::filesystem::path &file_path,
                                                           const std::string &declaration_pattern) {
  auto lines_result = read_file_lines(file_path);
  if (!lines_result) {
    return std::unexpected{
        fmt::format("Failed to read file {}: {}", file_path.string(), lines_result.error())};
  }

  for (const auto &lines = lines_result.value(); const auto &line : lines) {
    if (line.find(declaration_pattern) != std::string::npos) {
      return true;
    }
  }

  return false;
}

Result<size_t>
TextualHeaderFileParser::find_insertion_point(const std::filesystem::path &file_path) {
  auto lines_result = read_file_lines(file_path);
  if (!lines_result) {
    return std::unexpected{
        fmt::format("Failed to read file {}: {}", file_path.string(), lines_result.error())};
  }

  const auto &lines = lines_result.value();

  // For simple appending, we'll insert at the end of the file
  // In a more sophisticated implementation, we might look for
  // include guards or specific comment markers
  return lines.size();
}

Result<bool>
TextualHeaderFileParser::validate_header_structure(const std::filesystem::path &file_path) {
  // Check if file exists and is readable
  if (!std::filesystem::exists(file_path)) {
    return std::unexpected{fmt::format("File does not exist: {}", file_path.string())};
  }

  if (!std::filesystem::is_regular_file(file_path)) {
    return std::unexpected{fmt::format("Path is not a regular file: {}", file_path.string())};
  }

  // Try to read the file
  auto lines_result = read_file_lines(file_path);
  if (!lines_result) {
    return std::unexpected{
        fmt::format("Failed to read file {}: {}", file_path.string(), lines_result.error())};
  }

  const auto &lines = lines_result.value();

  // Basic validation: file should have some content
  if (lines.empty()) {
    return std::unexpected{fmt::format("File is empty: {}", file_path.string())};
  }

  // Check for obvious C header patterns
  bool has_c_content = false;
  for (const auto &line : lines) {
    const auto trimmed = trim_string(line);
    if (has_c_header_patterns(trimmed)) {
      has_c_content = true;
      break;
    }
  }

  if (!has_c_content) {
    return std::unexpected{
        fmt::format("File does not appear to contain C header content: {}", file_path.string())};
  }

  return true;
}

} // namespace porytiles2
#include "porytiles2/infra/services/project_c_source_file_appender.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "porytiles2/domain/services/c_source_file_modifier.hpp"
#include "porytiles2/domain/services/c_source_generator.hpp"
#include "porytiles2/infra/project/project_paths.hpp"
#include "porytiles2/templates/result.hpp"

#include <fmt/format.h>
#include <gsl/gsl>

namespace {

porytiles2::Result<std::string> read_file(const std::filesystem::path &file_path) {
  try {
    std::ifstream file{file_path};
    if (!file.is_open()) {
      return std::unexpected{fmt::format("Cannot open file for reading: {}", file_path.string())};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  } catch (const std::exception &e) {
    return std::unexpected{
        fmt::format("Exception reading file {}: {}", file_path.string(), e.what())};
  }
}

porytiles2::Result<void> write_file(const std::filesystem::path &file_path,
                                    const std::string &content) {
  try {
    std::ofstream file{file_path};
    if (!file.is_open()) {
      return std::unexpected{fmt::format("Cannot open file for writing: {}", file_path.string())};
    }

    file << content;
    if (file.fail()) {
      return std::unexpected{fmt::format("Failed to write to file: {}", file_path.string())};
    }

    return {};
  } catch (const std::exception &e) {
    return std::unexpected{
        fmt::format("Exception writing file {}: {}", file_path.string(), e.what())};
  }
}
porytiles2::Result<void> append_to_file(const std::filesystem::path &file_path,
                                        const std::string &content) {
  // Read existing file content
  auto existing_content_result = read_file(file_path);
  if (!existing_content_result) {
    return std::unexpected{fmt::format("Failed to read file {}: {}", file_path.string(),
                                       existing_content_result.error())};
  }

  // Append new content
  const auto updated_content = fmt::format("{}\n{}", existing_content_result.value(), content);

  // Write the updated content back to the file
  return write_file(file_path, updated_content);
}

} // namespace

namespace porytiles2 {

ProjectCSourceFileAppender::ProjectCSourceFileAppender(const gsl::not_null<ProjectPaths *> paths,
                                                       std::unique_ptr<CSourceGenerator> generator)
    : paths_{paths}, generator_{std::move(generator)} {}

Result<void>
ProjectCSourceFileAppender::append_to_graphics_header(const std::string &tileset_name) {
  const auto graphics_path = paths_->graphics_header();

  // Generate the palette and tile declarations
  const auto palette_declaration = generator_->generate_palette_declaration(tileset_name);
  const auto tile_declaration = generator_->generate_tile_declaration(tileset_name);

  // Combine the declarations with proper spacing
  const auto content = fmt::format("{}\n\n{}\n", palette_declaration, tile_declaration);

  return append_to_file(graphics_path, content);
}

Result<void> ProjectCSourceFileAppender::append_to_headers_header(const std::string &tileset_name) {
  const auto headers_path = paths_->headers_header();

  // Generate the tileset struct definition
  const auto struct_definition = generator_->generate_tileset_struct_definition(tileset_name);

  // Add the struct definition with proper spacing
  const auto content = fmt::format("{}\n", struct_definition);

  return append_to_file(headers_path, content);
}

Result<void>
ProjectCSourceFileAppender::append_to_metatiles_header(const std::string &tileset_name) {
  const auto metatiles_path = paths_->metatiles_header();

  // Generate the metatile and attribute declarations
  const auto metatile_declaration = generator_->generate_metatile_declaration(tileset_name);
  const auto attribute_declaration =
      generator_->generate_metatile_attribute_declaration(tileset_name);

  // Combine the declarations with proper spacing
  const auto content = fmt::format("{}\n\n{}\n", metatile_declaration, attribute_declaration);

  return append_to_file(metatiles_path, content);
}

Result<void>
ProjectCSourceFileAppender::append_tileset_declarations(const std::string &tileset_name) {
  // Perform all three operations in sequence
  if (auto result = append_to_graphics_header(tileset_name); !result) {
    return std::unexpected{fmt::format("Failed to append to graphics.h: {}", result.error())};
  }

  if (auto result = append_to_headers_header(tileset_name); !result) {
    return std::unexpected{fmt::format("Failed to append to headers.h: {}", result.error())};
  }

  if (auto result = append_to_metatiles_header(tileset_name); !result) {
    return std::unexpected{fmt::format("Failed to append to metatiles.h: {}", result.error())};
  }

  return {};
}

} // namespace porytiles2

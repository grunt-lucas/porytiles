#include "porytiles2/infra/services/ProjectCSourceFileAppender.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "porytiles2/domain/services/CSourceFileModifier.hpp"
#include "porytiles2/domain/services/CSourceGenerator.hpp"
#include "porytiles2/infra/project/ProjectPaths.hpp"
#include "porytiles2/templates/Result.hpp"

#include <fmt/format.h>
#include <gsl/gsl>

namespace porytiles {

ProjectCSourceFileAppender::ProjectCSourceFileAppender(const gsl::not_null<ProjectPaths *> paths,
                                                       std::unique_ptr<CSourceGenerator> generator)
    : paths_{paths}, generator_{std::move(generator)} {}

Result<void> ProjectCSourceFileAppender::AppendToGraphicsHeader(const std::string &tileset_name) {
  const auto graphics_path = paths_->GraphicsHeader();

  // Generate the palette and tile declarations
  const auto palette_declaration = generator_->GeneratePaletteDeclaration(tileset_name);
  const auto tile_declaration = generator_->GenerateTileDeclaration(tileset_name);

  // Combine the declarations with proper spacing
  const auto content = fmt::format("{}\n\n{}\n", palette_declaration, tile_declaration);

  return AppendToFile(graphics_path, content);
}

Result<void> ProjectCSourceFileAppender::AppendToHeadersHeader(const std::string &tileset_name) {
  const auto headers_path = paths_->HeadersHeader();

  // Generate the tileset struct definition
  const auto struct_definition = generator_->GenerateTilesetStructDefinition(tileset_name);

  // Add the struct definition with proper spacing
  const auto content = fmt::format("{}\n", struct_definition);

  return AppendToFile(headers_path, content);
}

Result<void> ProjectCSourceFileAppender::AppendToMetatilesHeader(const std::string &tileset_name) {
  const auto metatiles_path = paths_->MetatilesHeader();

  // Generate the metatile and attribute declarations
  const auto metatile_declaration = generator_->GenerateMetatileDeclaration(tileset_name);
  const auto attribute_declaration = generator_->GenerateMetatileAttributeDeclaration(tileset_name);

  // Combine the declarations with proper spacing
  const auto content = fmt::format("{}\n\n{}\n", metatile_declaration, attribute_declaration);

  return AppendToFile(metatiles_path, content);
}

Result<void>
ProjectCSourceFileAppender::AppendTilesetDeclarations(const std::string &tileset_name) {
  // Perform all three operations in sequence
  if (auto result = AppendToGraphicsHeader(tileset_name); !result) {
    return std::unexpected{fmt::format("Failed to append to graphics.h: {}", result.error())};
  }

  if (auto result = AppendToHeadersHeader(tileset_name); !result) {
    return std::unexpected{fmt::format("Failed to append to headers.h: {}", result.error())};
  }

  if (auto result = AppendToMetatilesHeader(tileset_name); !result) {
    return std::unexpected{fmt::format("Failed to append to metatiles.h: {}", result.error())};
  }

  return {};
}

Result<void> ProjectCSourceFileAppender::AppendToFile(const std::filesystem::path &file_path,
                                                      const std::string &content) {
  // Read existing file content
  auto existing_content_result = ReadFile(file_path);
  if (!existing_content_result) {
    return std::unexpected{fmt::format("Failed to read file {}: {}", file_path.string(),
                                       existing_content_result.error())};
  }

  // Append new content
  const auto updated_content = fmt::format("{}\n{}", existing_content_result.value(), content);

  // Write the updated content back to the file
  return WriteFile(file_path, updated_content);
}

Result<std::string> ProjectCSourceFileAppender::ReadFile(const std::filesystem::path &file_path) {
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

Result<void> ProjectCSourceFileAppender::WriteFile(const std::filesystem::path &file_path,
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

} // namespace porytiles
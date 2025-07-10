#include "porytiles2/infra/project/ProjectPaths.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace porytiles2 {

static const std::filesystem::path kPrimaryTilesetsRelativePath =
    std::filesystem::path{"data"} / "tilesets" / "primary";
static const std::filesystem::path kSecondaryTilesetsRelativePath =
    std::filesystem::path{"data"} / "tilesets" / "secondary";
static const std::filesystem::path kBehaviorsHeaderRelativePath =
    std::filesystem::path{"include"} / "constants";
static const std::string kBehaviorsHeaderFileName = "metatile_behaviors.h";
static const std::filesystem::path kTilesetSourceHeadersRelativePath =
    std::filesystem::path{"src"} / "data" / "tilesets";
static const std::string kGraphicsHeaderFileName = "graphics.h";
static const std::string kHeadersHeaderFileName = "headers.h";
static const std::string kMetatilesHeaderFileName = "metatiles.h";

std::filesystem::path ProjectPaths::primary_tileset_directory(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset;
}

std::filesystem::path ProjectPaths::primary_bottom_png(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset / "porytiles" / "bottom.png";
}

std::filesystem::path ProjectPaths::primary_middle_png(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset / "porytiles" / "middle.png";
}

std::filesystem::path ProjectPaths::primary_top_png(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset / "porytiles" / "top.png";
}

std::filesystem::path ProjectPaths::secondary_tileset_directory(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset;
}

std::filesystem::path ProjectPaths::secondary_bottom_png(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset / "porytiles" / "bottom.png";
}

std::filesystem::path ProjectPaths::secondary_middle_png(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset / "porytiles" / "middle.png";
}

std::filesystem::path ProjectPaths::secondary_top_png(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset / "porytiles" / "top.png";
}

std::filesystem::path ProjectPaths::behaviors_header() const {
  const auto header_path = behaviors_header_override_path_.value_or(kBehaviorsHeaderRelativePath);
  const auto header_file = behaviors_header_override_file_.value_or(kBehaviorsHeaderFileName);
  return project_root_ / header_path / header_file;
}

void ProjectPaths::set_behaviors_header_override_path(std::filesystem::path override) {
  behaviors_header_override_path_.emplace(std::move(override));
}

void ProjectPaths::set_behaviors_header_override_file(std::string override) {
  behaviors_header_override_file_.emplace(std::move(override));
}

std::filesystem::path ProjectPaths::graphics_header() const {
  return project_root_ / kTilesetSourceHeadersRelativePath / kGraphicsHeaderFileName;
}

std::filesystem::path ProjectPaths::headers_header() const {
  return project_root_ / kTilesetSourceHeadersRelativePath / kHeadersHeaderFileName;
}

std::filesystem::path ProjectPaths::metatiles_header() const {
  return project_root_ / kTilesetSourceHeadersRelativePath / kMetatilesHeaderFileName;
}

} // namespace porytiles2

#include "porytiles2/infra/project/ProjectPaths.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace porytiles {

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

std::filesystem::path ProjectPaths::PrimaryTilesetDirectory(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset;
}

std::filesystem::path ProjectPaths::PrimaryBottomPng(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset / "porytiles" / "bottom.png";
}

std::filesystem::path ProjectPaths::PrimaryMiddlePng(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset / "porytiles" / "middle.png";
}

std::filesystem::path ProjectPaths::PrimaryTopPng(const std::string &tileset) const {
  return project_root_ / kPrimaryTilesetsRelativePath / tileset / "porytiles" / "top.png";
}

std::filesystem::path ProjectPaths::SecondaryTilesetDirectory(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset;
}

std::filesystem::path ProjectPaths::SecondaryBottomPng(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset / "porytiles" / "bottom.png";
}

std::filesystem::path ProjectPaths::SecondaryMiddlePng(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset / "porytiles" / "middle.png";
}

std::filesystem::path ProjectPaths::SecondaryTopPng(const std::string &tileset) const {
  return project_root_ / kSecondaryTilesetsRelativePath / tileset / "porytiles" / "top.png";
}

std::filesystem::path ProjectPaths::BehaviorsHeader() const {
  const auto header_path = behaviors_header_override_path_.value_or(kBehaviorsHeaderRelativePath);
  const auto header_file = behaviors_header_override_file_.value_or(kBehaviorsHeaderFileName);
  return project_root_ / header_path / header_file;
}

void ProjectPaths::SetBehaviorsHeaderOverridePath(std::filesystem::path override) {
  behaviors_header_override_path_.emplace(std::move(override));
}

void ProjectPaths::SetBehaviorsHeaderOverrideFile(std::string override) {
  behaviors_header_override_file_.emplace(std::move(override));
}

std::filesystem::path ProjectPaths::GraphicsHeader() const {
  return project_root_ / kTilesetSourceHeadersRelativePath / kGraphicsHeaderFileName;
}

std::filesystem::path ProjectPaths::HeadersHeader() const {
  return project_root_ / kTilesetSourceHeadersRelativePath / kHeadersHeaderFileName;
}

std::filesystem::path ProjectPaths::MetatilesHeader() const {
  return project_root_ / kTilesetSourceHeadersRelativePath / kMetatilesHeaderFileName;
}

} // namespace porytiles

#include "porytiles2/infra/project/ProjectPaths.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace porytiles {

static const std::filesystem::path kTilesetPrimaryRootPath =
    std::filesystem::path{"data"} / "tilesets" / "primary";
static const std::filesystem::path kTilesetSecondaryRootPath =
    std::filesystem::path{"data"} / "tilesets" / "secondary";
static const std::filesystem::path kBehaviorsHeaderPath =
    std::filesystem::path{"include"} / "constants";
static const std::string kBehaviorsHeaderFile = "metatile_behaviors.h";

std::filesystem::path
ProjectPaths::PrimaryBottomPng(const std::string &tileset) const {
  return kTilesetPrimaryRootPath / tileset / "porytiles" / "bottom.png";
}

std::filesystem::path
ProjectPaths::PrimaryMiddlePng(const std::string &tileset) const {
  return kTilesetPrimaryRootPath / tileset / "porytiles" / "middle.png";
}

std::filesystem::path
ProjectPaths::PrimaryTopPng(const std::string &tileset) const {
  return kTilesetPrimaryRootPath / tileset / "porytiles" / "top.png";
}

std::filesystem::path ProjectPaths::BehaviorsHeader() const {
  const auto header_path =
      behaviors_header_override_path_.value_or(kBehaviorsHeaderPath);
  const auto header_file =
      behaviors_header_override_file_.value_or(kBehaviorsHeaderFile);
  return std::filesystem::path{project_root_} / header_path / header_file;
}

void ProjectPaths::SetBehaviorsHeaderOverridePath(
    std::filesystem::path override) {
  behaviors_header_override_path_.emplace(std::move(override));
}

void ProjectPaths::SetBehaviorsHeaderOverrideFile(std::string override) {
  behaviors_header_override_file_.emplace(std::move(override));
}

} // namespace porytiles

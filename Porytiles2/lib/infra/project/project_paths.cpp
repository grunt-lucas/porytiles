#include <porytiles2/infra/project/project_paths.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace porytiles {

static constexpr std::string kBehaviorsHeaderFile = "metatile_behaviors.h";
static const std::filesystem::path kBehaviorsHeaderPath = std::filesystem::path{"include"} / "constants";

std::filesystem::path ProjectPaths::BottomPng(const std::string &tileset) const {}

std::filesystem::path ProjectPaths::BehaviorsHeader() const {
    const auto header_path = behaviors_header_override_path_.value_or(kBehaviorsHeaderPath);
    const auto header_file = behaviors_header_override_file_.value_or(kBehaviorsHeaderFile);
    return std::filesystem::path{project_root_} / header_path / header_file;
}

void ProjectPaths::SetBehaviorsHeaderOverridePath(std::filesystem::path override) {
    behaviors_header_override_path_.emplace(std::move(override));
}

void ProjectPaths::SetBehaviorsHeaderOverrideFile(std::string override) {
    behaviors_header_override_file_.emplace(std::move(override));
}

} // namespace porytiles

#include "porytiles2/infra/services/anim_yaml_parser.hpp"

#include <fstream>

#include "yaml-cpp/yaml.h"

#include "porytiles2/utilities/result/chainable_result.hpp"

namespace {

using namespace porytiles2;

AnimationParams parse_animation_params(const YAML::Node &node)
{
    AnimationParams params;

    /*
     * TODO: would be nice to warn users here about unknown keys. That way, if they have a simple typo (e.g.
     * 'farme_factor' instead of 'frame_factor') they won't be super confused as to why the animation param isn't being
     * set properly.
     */

    if (node["frame_factor"]) {
        params.frame_factor(node["frame_factor"].as<std::size_t>());
    }

    if (node["frame_offset"]) {
        params.frame_offset(node["frame_offset"].as<std::size_t>());
    }

    if (node["frames"]) {
        std::vector<std::size_t> frames;
        for (const auto &frame : node["frames"]) {
            frames.push_back(frame.as<std::size_t>());
        }
        params.frames(std::move(frames));
    }

    if (node["counter_max"]) {
        params.counter_max(node["counter_max"].as<std::size_t>());
    }

    // Note: tile_offset and tile_count are NOT read from anim.yaml
    // They are computed during compilation and stored in generated_anim_code.h

    return params;
}

YAML::Node serialize_animation_params(const AnimationParams &params)
{
    YAML::Node node;

    // Only write non-default values to keep the file clean
    if (params.frame_factor() != anim::default_frame_factor) {
        node["frame_factor"] = params.frame_factor();
    }

    if (params.frame_offset() != anim::default_frame_offset) {
        node["frame_offset"] = params.frame_offset();
    }

    // Always write frames array since it's the core animation definition
    YAML::Node frames_node;
    for (const auto frame : params.frames()) {
        frames_node.push_back(frame);
    }
    node["frames"] = frames_node;

    if (params.counter_max() != anim::default_counter_max) {
        node["counter_max"] = params.counter_max();
    }

    // Note: tile_offset and tile_count are NOT written to anim.yaml
    // They are stored in generated_anim_code.h

    return node;
}

} // namespace

namespace porytiles2 {

ChainableResult<std::map<std::string, AnimationParams>>
AnimYamlParser::parse(const std::filesystem::path &yaml_path) const
{
    /*
     * TODO: improve error message formatting
     */

    if (!std::filesystem::exists(yaml_path)) {
        return FormattableError{
            std::vector<std::string>{"anim.yaml file not found", "expected file at: {}"},
            std::vector<std::vector<FormatParam>>{{}, {FormatParam{yaml_path.string(), Style::bold}}}};
    }

    try {
        YAML::Node root = YAML::LoadFile(yaml_path.string());

        std::map<std::string, AnimationParams> result;

        if (!root.IsMap()) {
            return FormattableError{
                std::vector<std::string>{"invalid anim.yaml format", "expected a YAML map at the root level"},
                std::vector<std::vector<FormatParam>>{{}, {}}};
        }

        for (const auto &entry : root) {
            const auto anim_name = entry.first.as<std::string>();
            const YAML::Node &anim_node = entry.second;

            if (!anim_node.IsMap()) {
                return FormattableError{
                    std::vector<std::string>{
                        "invalid animation entry in anim.yaml",
                        "animation '{}' should be a YAML map with frame_factor, frame_offset, frames fields"},
                    std::vector<std::vector<FormatParam>>{{}, {FormatParam{anim_name, Style::bold}}}};
            }

            result.insert({anim_name, parse_animation_params(anim_node)});
        }

        return result;
    }
    catch (const YAML::Exception &e) {
        return FormattableError{
            std::vector<std::string>{"failed to parse anim.yaml", "YAML error: {}"},
            std::vector<std::vector<FormatParam>>{{}, {FormatParam{e.what()}}}};
    }
}

ChainableResult<void> AnimYamlParser::write(
    const std::filesystem::path &yaml_path, const std::map<std::string, AnimationParams> &params) const
{
    /*
     * TODO: improve error message formatting
     */

    try {
        // Create parent directories if they don't exist
        if (yaml_path.has_parent_path()) {
            std::filesystem::create_directories(yaml_path.parent_path());
        }

        YAML::Node root;
        for (const auto &[name, anim_params] : params) {
            root[name] = serialize_animation_params(anim_params);
        }

        std::ofstream out(yaml_path);
        if (!out) {
            return FormattableError{
                std::vector<std::string>{"failed to open anim.yaml for writing", "path: {}"},
                std::vector<std::vector<FormatParam>>{{}, {FormatParam{yaml_path.string(), Style::bold}}}};
        }

        out << root;
        out.close();

        if (!out) {
            return FormattableError{
                std::vector<std::string>{"failed to write anim.yaml", "error occurred while writing to: {}"},
                std::vector<std::vector<FormatParam>>{{}, {FormatParam{yaml_path.string(), Style::bold}}}};
        }

        return {};
    }
    catch (const YAML::Exception &e) {
        return FormattableError{
            std::vector<std::string>{"failed to serialize anim.yaml", "YAML error: {}"},
            std::vector<std::vector<FormatParam>>{{}, {FormatParam{e.what()}}}};
    }
}

} // namespace porytiles2
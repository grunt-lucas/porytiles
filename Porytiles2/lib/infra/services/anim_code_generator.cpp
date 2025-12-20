#include "porytiles2/infra/services/anim_code_generator.hpp"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <sstream>

#include "fmt/format.h"

#include "porytiles2/utilities/string_utils.hpp"

namespace porytiles2 {

ChainableResult<std::string> AnimCodeGenerator::generate(
    const std::string &tileset_name,
    const std::filesystem::path &tileset_path_from_project_root,
    const std::map<std::string, AnimationParams> &animations,
    bool is_primary) const
{
    if (animations.empty()) {
        return FormattableError{"no animations to generate code for"};
    }

    std::ostringstream out;

    const std::string pascal_tileset_name = to_pascal_case(tileset_name);

    // Generate header guard
    const std::string guard_name = fmt::format("GUARD_GENERATED_ANIM_CODE_{}_H", pascal_tileset_name);
    out << fmt::format("#ifndef {}\n", guard_name);
    out << fmt::format("#define {}\n\n", guard_name);

    // Generate INCBIN macro if not defined
    out << "// Ensure INCBIN_U16 is available\n";
    out << "#ifndef INCBIN_U16\n";
    out << "#include \"gba/defines.h\"\n";
    out << "#endif\n\n";

    // Generate INCBIN statements for all animations
    out << "// ============================================\n";
    out << "// Frame Data (INCBIN statements)\n";
    out << "// ============================================\n\n";

    for (const auto &[anim_name, params] : animations) {
        out << generate_incbin_statements(pascal_tileset_name, tileset_path_from_project_root, anim_name, params);
        out << "\n";
    }

    // Generate frame pointer arrays
    out << "// ============================================\n";
    out << "// Frame Pointer Arrays\n";
    out << "// ============================================\n\n";

    for (const auto &[anim_name, params] : animations) {
        out << generate_frame_array(pascal_tileset_name, anim_name, params);
        out << "\n";
    }

    // Generate Queue functions
    out << "// ============================================\n";
    out << "// Queue Functions\n";
    out << "// ============================================\n\n";

    for (const auto &[anim_name, params] : animations) {
        out << generate_queue_function(pascal_tileset_name, anim_name, params);
        out << "\n";
    }

    // Generate driver function
    out << "// ============================================\n";
    out << "// Driver Function\n";
    out << "// ============================================\n\n";

    out << generate_driver_function(pascal_tileset_name, animations);
    out << "\n";

    // Generate init function
    out << "// ============================================\n";
    out << "// Init Function\n";
    out << "// ============================================\n\n";

    out << generate_init_function(pascal_tileset_name, animations, is_primary);

    // Close header guard
    out << fmt::format("\n#endif // {}\n", guard_name);

    return out.str();
}

std::string AnimCodeGenerator::generate_incbin_statements(
    const std::string &tileset_name,
    const std::filesystem::path &tileset_path_from_project_root,
    const std::string &anim_name,
    const AnimationParams &params) const
{
    std::ostringstream out;

    const std::string pascal_anim_name = to_pascal_case(anim_name);
    const std::size_t max_frame_idx = find_max_frame_index(params.frames());

    // Generate INCBIN for each unique frame referenced
    for (std::size_t frame_idx = 0; frame_idx <= max_frame_idx; ++frame_idx) {
        const std::string frame_file =
            (tileset_path_from_project_root / "anim" / anim_name / fmt::format("{}.4bpp", frame_idx)).string();

        const auto statement = fmt::format(
            "const u16 gTilesetAnims_PorytilesManaged_{}_{}_Frame{}[] = INCBIN_U16(\"{}\");\n",
            tileset_name,
            pascal_anim_name,
            frame_idx,
            frame_file);

        out << statement;
    }

    return out.str();
}

std::string AnimCodeGenerator::generate_frame_array(
    const std::string &tileset_name, const std::string &anim_name, const AnimationParams &params) const
{
    std::ostringstream out;

    const std::string pascal_anim_name = to_pascal_case(anim_name);
    const std::string array_name = fmt::format("gTilesetAnims_PorytilesManaged_{}_{}", tileset_name, pascal_anim_name);

    out << fmt::format("const u16 *const {}[] = {{\n", array_name);

    for (std::size_t i = 0; i < params.frames().size(); ++i) {
        const std::size_t frame_idx = params.frames()[i];
        out << fmt::format(
            "    gTilesetAnims_PorytilesManaged_{}_{}_Frame{}", tileset_name, pascal_anim_name, frame_idx);
        if (i < params.frames().size() - 1) {
            out << ",";
        }
        out << "\n";
    }

    out << "};\n";

    return out.str();
}

std::string AnimCodeGenerator::generate_queue_function(
    const std::string &tileset_name, const std::string &anim_name, const AnimationParams &params) const
{
    std::ostringstream out;

    const std::string pascal_anim_name = to_pascal_case(anim_name);
    const std::string array_name = fmt::format("gTilesetAnims_PorytilesManaged_{}_{}", tileset_name, pascal_anim_name);
    const std::string func_name = fmt::format("QueueAnimTiles_PorytilesManaged_{}_{}", tileset_name, pascal_anim_name);

    out << fmt::format("static void {}(u16 timer)\n", func_name);
    out << "{\n";
    out << fmt::format("    u16 i = timer % ARRAY_COUNT({});\n", array_name);
    out << fmt::format(
        "    AppendTilesetAnimToBuffer({}[i], (u16 *)(BG_VRAM + TILE_OFFSET_4BPP({})), {} * TILE_SIZE_4BPP);\n",
        array_name,
        params.tile_offset(),
        params.tile_count());
    out << "}\n";

    return out.str();
}

std::string AnimCodeGenerator::generate_driver_function(
    const std::string &tileset_name, const std::map<std::string, AnimationParams> &animations) const
{
    std::ostringstream out;

    const std::string func_name = fmt::format("TilesetAnim_PorytilesManaged_{}", tileset_name);

    out << fmt::format("static void {}(u16 timer)\n", func_name);
    out << "{\n";

    // Group animations by their frame_factor
    std::map<std::size_t, std::vector<std::pair<std::string, AnimationParams>>> by_frame_factor;
    for (const auto &[anim_name, params] : animations) {
        by_frame_factor[params.frame_factor()].push_back({anim_name, params});
    }

    for (const auto &[frame_factor, anims] : by_frame_factor) {
        // Group by frame_offset within each frame_factor
        std::map<std::size_t, std::vector<std::pair<std::string, AnimationParams>>> by_offset;
        for (const auto &[anim_name, params] : anims) {
            by_offset[params.frame_offset()].push_back({anim_name, params});
        }

        for (const auto &[frame_offset, offset_anims] : by_offset) {
            out << fmt::format("    if (timer % {} == {})\n", frame_factor, frame_offset);
            out << "    {\n";

            for (const auto &anim_name : offset_anims | std::views::keys) {
                const std::string pascal_anim_name = to_pascal_case(anim_name);
                out << fmt::format(
                    "        QueueAnimTiles_PorytilesManaged_{}_{}(timer / {});\n",
                    tileset_name,
                    pascal_anim_name,
                    frame_factor);
            }

            out << "    }\n";
        }
    }

    out << "}\n";

    return out.str();
}

std::string AnimCodeGenerator::generate_init_function(
    const std::string &tileset_name, const std::map<std::string, AnimationParams> &animations, bool is_primary) const
{
    std::ostringstream out;

    const std::string init_func_name = fmt::format("InitTilesetAnim_PorytilesManaged_{}", tileset_name);
    const std::string driver_func_name = fmt::format("TilesetAnim_PorytilesManaged_{}", tileset_name);

    // Find the maximum counter_max value across all animations
    std::size_t max_counter_max = anim::default_counter_max;
    for (const auto &params : animations | std::views::values) {
        max_counter_max = std::max(max_counter_max, params.counter_max());
    }

    const std::string counter_var = is_primary ? "sPrimaryTilesetAnimCounter" : "sSecondaryTilesetAnimCounter";
    const std::string counter_max_var =
        is_primary ? "sPrimaryTilesetAnimCounterMax" : "sSecondaryTilesetAnimCounterMax";
    const std::string callback_var = is_primary ? "sPrimaryTilesetAnimCallback" : "sSecondaryTilesetAnimCallback";

    out << fmt::format("void {}(void)\n", init_func_name);
    out << "{\n";
    out << fmt::format("    {} = 0;\n", counter_var);
    out << fmt::format("    {} = {};\n", counter_max_var, max_counter_max);
    out << fmt::format("    {} = {};\n", callback_var, driver_func_name);
    out << "}\n";

    return out.str();
}

std::size_t AnimCodeGenerator::find_max_frame_index(const std::vector<std::size_t> &frames) const
{
    if (frames.empty()) {
        return 0;
    }
    return *std::ranges::max_element(frames);
}

} // namespace porytiles2
#include "porytiles2/infra/services/project_layout_metadata_provider.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "porytiles2/infra/models/project_layout_metadata.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

const std::filesystem::path layouts_json_rel_path = std::filesystem::path{"data"} / "layouts" / "layouts.json";

/**
 * @brief Ensures layout metadata has been parsed from layouts.json and cached.
 *
 * @details
 * Lazily parses the layouts.json file to extract all layout entries and the layouts table label. Results are cached to
 * avoid redundant parsing on subsequent calls. Builds into local temporaries and only commits to the mutable cache on
 * success, so a parse failure leaves the cache in a clean state for retry.
 *
 * @param project_root The root directory of the pokeemerald-style project
 * @param layouts_parsed Mutable flag tracking whether layouts have been parsed
 * @param layouts_table_label Mutable cache for the layouts table label string
 * @param layout_entries Mutable cache of parsed layout entries
 * @param layout_index Mutable index mapping both layout name and ID to entries
 * @param format Text formatter for styled output
 * @return Success or error result
 */
[[nodiscard]] ChainableResult<void> ensure_layouts_parsed(
    const std::filesystem::path &project_root,
    bool &layouts_parsed,
    std::string &layouts_table_label,
    std::vector<ProjectLayoutMetadata> &layout_entries,
    std::map<std::string, std::size_t> &layout_index,
    const TextFormatter *format)
{
    if (layouts_parsed) {
        return {};
    }

    const auto layouts_path = project_root / layouts_json_rel_path;

    std::ifstream file{layouts_path};
    if (!file.is_open()) {
        return FormattableError{
            format->format("Failed to open layouts file '{}'.", FormatParam{layouts_path.string(), Style::bold})};
    }

    nlohmann::json json_data;
    try {
        file >> json_data;
    }
    catch (const nlohmann::json::parse_error &e) {
        return FormattableError{format->format(
            "Failed to parse layouts JSON from '{}': {}.",
            FormatParam{layouts_path.string(), Style::bold},
            FormatParam{std::string{e.what()}})};
    }

    if (!json_data.contains("layouts_table_label")) {
        return FormattableError{format->format(
            "Layouts file '{}' missing required field 'layouts_table_label'.",
            FormatParam{layouts_path.string(), Style::bold})};
    }

    if (!json_data.contains("layouts")) {
        return FormattableError{format->format(
            "Layouts file '{}' missing required field 'layouts'.", FormatParam{layouts_path.string(), Style::bold})};
    }

    /*
     * Build into local temporaries so that a failure mid-loop does not leave the mutable cache in a partially populated
     * state. Only commit to the real cache on full success.
     */
    auto table_label = json_data.at("layouts_table_label").get<std::string>();

    const auto &layouts_array = json_data.at("layouts");
    std::vector<ProjectLayoutMetadata> entries;
    std::map<std::string, std::size_t> index;
    entries.reserve(layouts_array.size());

    for (const auto &entry : layouts_array) {
        try {
            auto id = entry.at("id").get<std::string>();
            auto name = entry.at("name").get<std::string>();
            auto width = entry.at("width").get<std::size_t>();
            auto height = entry.at("height").get<std::size_t>();
            auto primary_tileset = entry.at("primary_tileset").get<std::string>();
            auto secondary_tileset = entry.at("secondary_tileset").get<std::string>();
            auto border_fp = std::filesystem::path{entry.at("border_filepath").get<std::string>()};
            auto blockdata_fp = std::filesystem::path{entry.at("blockdata_filepath").get<std::string>()};

            const auto idx = entries.size();
            entries.push_back(
                ProjectLayoutMetadata{
                    std::move(id),
                    std::move(name),
                    width,
                    height,
                    std::move(primary_tileset),
                    std::move(secondary_tileset),
                    std::move(border_fp),
                    std::move(blockdata_fp)});

            index.emplace(entries.at(idx).id(), idx);
            index.emplace(entries.at(idx).name(), idx);
        }
        catch (const nlohmann::json::exception &e) {
            return FormattableError{format->format(
                "Malformed layout entry in '{}': {}.",
                FormatParam{layouts_path.string(), Style::bold},
                FormatParam{std::string{e.what()}})};
        }
    }

    layouts_table_label = std::move(table_label);
    layout_entries = std::move(entries);
    layout_index = std::move(index);
    layouts_parsed = true;
    return {};
}

/**
 * @brief Looks up a cached layout entry by name or ID.
 *
 * @details
 * Ensures layouts have been parsed, then searches the index for the given key. Returns a pointer into the cached layout
 * entries vector.
 *
 * @param layout_name_or_id The name or ID of the layout (e.g., "PetalburgCity_Layout" or "LAYOUT_PETALBURG_CITY")
 * @param project_root The root directory of the pokeemerald-style project
 * @param layouts_parsed Mutable flag tracking whether layouts have been parsed
 * @param layouts_table_label Mutable cache for the layouts table label string
 * @param layout_entries Mutable cache of parsed layout entries
 * @param layout_index Mutable index mapping both layout name and ID to entries
 * @param format Text formatter for styled output
 * @return Pointer to the cached layout entry, or error if not found
 */
[[nodiscard]] ChainableResult<const ProjectLayoutMetadata *> lookup_layout(
    const std::string &layout_name_or_id,
    const std::filesystem::path &project_root,
    bool &layouts_parsed,
    std::string &layouts_table_label,
    std::vector<ProjectLayoutMetadata> &layout_entries,
    std::map<std::string, std::size_t> &layout_index,
    const TextFormatter *format)
{
    if (const auto ensure_result = ensure_layouts_parsed(
            project_root, layouts_parsed, layouts_table_label, layout_entries, layout_index, format);
        !ensure_result.has_value()) {
        return ChainableResult<const ProjectLayoutMetadata *>{
            FormattableError{
                format->format("Failed to look up layout '{}'.", FormatParam{layout_name_or_id, Style::bold})},
            ensure_result};
    }

    if (!layout_index.contains(layout_name_or_id)) {
        return FormattableError{
            format->format("Layout '{}' not found in layouts.json.", FormatParam{layout_name_or_id, Style::bold})};
    }

    return &layout_entries.at(layout_index.at(layout_name_or_id));
}

} // namespace

namespace porytiles2 {

bool ProjectLayoutMetadataProvider::exists(const std::string &layout_name_or_id) const
{
    if (const auto result = ensure_layouts_parsed(
            project_root_, layouts_parsed_, layouts_table_label_, layout_entries_, layout_index_, format_);
        !result.has_value()) {
        return false;
    }
    return layout_index_.contains(layout_name_or_id);
}

ChainableResult<std::size_t> ProjectLayoutMetadataProvider::width(const std::string &layout_name_or_id) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        layout,
        ::lookup_layout(
            layout_name_or_id,
            project_root_,
            layouts_parsed_,
            layouts_table_label_,
            layout_entries_,
            layout_index_,
            format_),
        std::size_t);
    return layout->width();
}

ChainableResult<std::size_t> ProjectLayoutMetadataProvider::height(const std::string &layout_name_or_id) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        layout,
        ::lookup_layout(
            layout_name_or_id,
            project_root_,
            layouts_parsed_,
            layouts_table_label_,
            layout_entries_,
            layout_index_,
            format_),
        std::size_t);
    return layout->height();
}

ChainableResult<std::string> ProjectLayoutMetadataProvider::primary_tileset(const std::string &layout_name_or_id) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        layout,
        ::lookup_layout(
            layout_name_or_id,
            project_root_,
            layouts_parsed_,
            layouts_table_label_,
            layout_entries_,
            layout_index_,
            format_),
        std::string);
    return layout->primary_tileset();
}

ChainableResult<std::string>
ProjectLayoutMetadataProvider::secondary_tileset(const std::string &layout_name_or_id) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        layout,
        ::lookup_layout(
            layout_name_or_id,
            project_root_,
            layouts_parsed_,
            layouts_table_label_,
            layout_entries_,
            layout_index_,
            format_),
        std::string);
    return layout->secondary_tileset();
}

ChainableResult<std::set<std::string>> ProjectLayoutMetadataProvider::layout_names() const
{
    PT_TRY_CALL_CHAIN_ERR(
        ensure_layouts_parsed(
            project_root_, layouts_parsed_, layouts_table_label_, layout_entries_, layout_index_, format_),
        std::set<std::string>,
        "Failed to enumerate layout names.");

    std::set<std::string> names;
    for (const auto &entry : layout_entries_) {
        names.insert(entry.name());
    }
    return names;
}

ChainableResult<std::set<std::string>> ProjectLayoutMetadataProvider::layout_ids() const
{
    PT_TRY_CALL_CHAIN_ERR(
        ensure_layouts_parsed(
            project_root_, layouts_parsed_, layouts_table_label_, layout_entries_, layout_index_, format_),
        std::set<std::string>,
        "Failed to enumerate layout IDs.");

    std::set<std::string> ids;
    for (const auto &entry : layout_entries_) {
        ids.insert(entry.id());
    }
    return ids;
}

ChainableResult<std::string> ProjectLayoutMetadataProvider::layouts_table_label() const
{
    PT_TRY_CALL_CHAIN_ERR(
        ensure_layouts_parsed(
            project_root_, layouts_parsed_, layouts_table_label_, layout_entries_, layout_index_, format_),
        std::string,
        "Failed to get layouts table label.");
    return layouts_table_label_;
}

ChainableResult<std::filesystem::path>
ProjectLayoutMetadataProvider::border_filepath(const std::string &layout_name_or_id) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        layout,
        ::lookup_layout(
            layout_name_or_id,
            project_root_,
            layouts_parsed_,
            layouts_table_label_,
            layout_entries_,
            layout_index_,
            format_),
        std::filesystem::path);
    return project_root_ / layout->border_filepath();
}

ChainableResult<std::filesystem::path>
ProjectLayoutMetadataProvider::blockdata_filepath(const std::string &layout_name_or_id) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        layout,
        ::lookup_layout(
            layout_name_or_id,
            project_root_,
            layouts_parsed_,
            layouts_table_label_,
            layout_entries_,
            layout_index_,
            format_),
        std::filesystem::path);
    return project_root_ / layout->blockdata_filepath();
}

void ProjectLayoutMetadataProvider::invalidate_metadata_cache() const
{
    layouts_parsed_ = false;
    layouts_table_label_.clear();
    layout_entries_.clear();
    layout_index_.clear();
}

} // namespace porytiles2

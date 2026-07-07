#include "porytiles/infra/services/attributes_csv_loader.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>

#include "porytiles/domain/models/layer.hpp"
#include "porytiles/utilities/parse_int.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace {

using namespace porytiles;

enum class CsvFormat { emerald, firered };

struct CsvRow {
    std::size_t metatile_id;
    std::string behavior;
    std::string terrain_type;
    std::string encounter_type;
    std::optional<std::string> layer_type_token; // raw layerType cell, nullopt when the column or cell is blank
};

/**
 * @brief Extracts the trimmed layerType cell at a fixed column index, or nullopt when the cell is blank/absent.
 *
 * @details
 * The split() helper keeps trailing empty fields, so a blank cell arrives as an empty string; a row that simply omits
 * the trailing comma has fewer columns than @p index. Both cases mean "no explicit layer type" and map to nullopt.
 */
[[nodiscard]] std::optional<std::string>
extract_layer_type_token(const std::vector<std::string> &columns, bool has_layer_type_column, std::size_t index)
{
    if (!has_layer_type_column || columns.size() <= index) {
        return std::nullopt;
    }
    std::string token = columns[index];
    trim(token);
    if (token.empty()) {
        return std::nullopt;
    }
    return token;
}

ChainableResult<CsvRow> parse_emerald_csv_row(
    const std::string &line,
    std::size_t line_index,
    const std::filesystem::path &path,
    const std::vector<std::string> &all_lines,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer,
    bool has_layer_type_column)
{
    auto columns = split(line, ",");

    if (columns.size() < 2) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected at least 2 columns (id,behavior), found {}",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{columns.size()}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    trim(columns[0]);
    trim(columns[1]);

    auto id_result = parse_int<int>(columns[0], 0);
    if (!id_result.has_value()) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: invalid metatile id '{}': {}",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{columns[0], Style::bold},
            FormatParam{id_result.error()}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    if (id_result.value() < 0) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: metatile id '{}' cannot be negative",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{columns[0], Style::bold}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    // Emerald layerType column, when present, sits at index 2 (after id, behavior).
    return CsvRow{
        static_cast<std::size_t>(id_result.value()),
        columns[1],
        "",
        "",
        extract_layer_type_token(columns, has_layer_type_column, 2)};
}

ChainableResult<CsvRow> parse_firered_csv_row(
    const std::string &line,
    std::size_t line_index,
    const std::filesystem::path &path,
    const std::vector<std::string> &all_lines,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer,
    bool has_layer_type_column)
{
    auto columns = split(line, ",");

    if (columns.size() < 4) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected 4 columns (id,behavior,terrainType,encounterType), found {}",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{columns.size()}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    trim(columns[0]);
    trim(columns[1]);
    trim(columns[2]);
    trim(columns[3]);

    auto id_result = parse_int<int>(columns[0], 0);
    if (!id_result.has_value()) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: invalid metatile id '{}': {}",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{columns[0], Style::bold},
            FormatParam{id_result.error()}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    if (id_result.value() < 0) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: metatile id '{}' cannot be negative",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{columns[0], Style::bold}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    // FireRed layerType column, when present, sits at index 4 (after id, behavior, terrainType, encounterType).
    return CsvRow{
        static_cast<std::size_t>(id_result.value()),
        columns[1],
        columns[2],
        columns[3],
        extract_layer_type_token(columns, has_layer_type_column, 4)};
}

ChainableResult<std::map<std::size_t, MetatileAttribute>> parse_attributes_csv(
    const std::filesystem::path &path,
    const EnumMapProvider &behavior_map,
    std::optional<BaseGame> base_game,
    const EnumMapProvider *terrain_map,
    const EnumMapProvider *encounter_map,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer,
    bool write_layer_type_column,
    const UserDiagnostics &diag)
{
    if (!exists(path)) {
        return FormattableError{"{}: file does not exist.", FormatParam{path.string(), Style::bold}};
    }

    // Slurp entire file into vector for FileHighlightPrinter support
    std::vector<std::string> lines{};
    {
        std::ifstream stream{path};
        std::string line_buf{};
        while (std::getline(stream, line_buf)) {
            std::ignore = trim_line_ending(line_buf);
            lines.push_back(line_buf);
        }
    }

    if (lines.empty()) {
        return FormattableError{
            "{}: file is empty, expected header 'id,behavior' or 'id,behavior,terrainType,encounterType'",
            FormatParam{path.string(), Style::bold}};
    }

    // Validate header line (index 0) and auto-detect format
    auto header_columns = split(lines[0], ",");
    for (auto &col : header_columns) {
        trim(col);
    }

    CsvFormat csv_format{};

    if (header_columns.size() >= 4 && header_columns[0] == "id" && header_columns[1] == "behavior" &&
        header_columns[2] == "terrainType" && header_columns[3] == "encounterType") {
        csv_format = CsvFormat::firered;
    }
    else if (header_columns.size() >= 2 && header_columns[0] == "id" && header_columns[1] == "behavior") {
        csv_format = CsvFormat::emerald;
    }
    else {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: invalid header, expected 'id,behavior' or 'id,behavior,terrainType,encounterType' but found '{}'",
            FormatParam{path.string(), Style::bold},
            FormatParam{"1", Style::bold},
            FormatParam{lines[0], Style::bold}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(lines, std::vector<std::size_t>{0}));
        return FormattableError{std::move(err_lines)};
    }

    // Validate detected format against base game if provided
    if (base_game.has_value()) {
        if (csv_format == CsvFormat::firered && base_game.value() != BaseGame::pokefirered) {
            std::vector<std::string> err_lines{};
            err_lines.push_back(format.format(
                "{}:{}: CSV has FireRed format (id,behavior,terrainType,encounterType) but project base game is '{}'",
                FormatParam{path.string(), Style::bold},
                FormatParam{"1", Style::bold},
                FormatParam{to_string(base_game.value()), Style::bold}));
            err_lines.emplace_back();
            err_lines.append_range(file_printer.print(lines, std::vector<std::size_t>{0}));
            return FormattableError{std::move(err_lines)};
        }
        if (csv_format == CsvFormat::emerald && base_game.value() == BaseGame::pokefirered) {
            std::vector<std::string> err_lines{};
            err_lines.push_back(format.format(
                "{}:{}: CSV has Emerald format (id,behavior) but project base game is '{}'",
                FormatParam{path.string(), Style::bold},
                FormatParam{"1", Style::bold},
                FormatParam{to_string(base_game.value()), Style::bold}));
            err_lines.emplace_back();
            err_lines.append_range(file_printer.print(lines, std::vector<std::size_t>{0}));
            return FormattableError{std::move(err_lines)};
        }
    }

    // Validate provider availability for FireRed format
    if (csv_format == CsvFormat::firered) {
        if (terrain_map == nullptr) {
            panic("FireRed CSV format requires a terrain type provider but none was provided.");
        }
        if (encounter_map == nullptr) {
            panic("FireRed CSV format requires an encounter type provider but none was provided.");
        }
    }

    // Detect an optional trailing layerType column at its fixed position for the detected format (index 2 for emerald,
    // index 4 for firered). We always detect it; the knob decides whether its values are applied. Other unknown
    // trailing columns stay tolerated.
    const std::size_t layer_type_index = csv_format == CsvFormat::firered ? 4 : 2;
    const bool has_layer_type_column =
        header_columns.size() > layer_type_index && header_columns[layer_type_index] == "layerType";

    // Knob off but the column is present: ignore the values and say so once for the whole file.
    if (has_layer_type_column && !write_layer_type_column) {
        diag.warning(
            "layer-type-column",
            std::vector<std::string>{
                format.format(
                    "{}: a layerType column is present but write_layer_type_column is off; its values are ignored and "
                    "layer types will be inferred.",
                    FormatParam{path.string(), Style::bold}),
                format.format(
                    "set {} to apply the column.",
                    FormatParam{"fieldmap.write_layer_type_column: true", Style::bold})});
    }

    // Parse data rows (starting at index 1)
    std::map<std::size_t, MetatileAttribute> result{};
    std::unordered_map<std::size_t, std::size_t> id_to_line_index{};

    // Applies a filled layerType cell as an explicit override, when the column is present and the knob is on. A blank
    // cell (nullopt token) leaves the layer type inferred. A bad token is a hard error with file context.
    auto apply_explicit_layer_type =
        [&](MetatileAttribute &attribute, const CsvRow &row, std::size_t line_index) -> ChainableResult<void> {
        if (!has_layer_type_column || !write_layer_type_column || !row.layer_type_token.has_value()) {
            return {};
        }
        auto layer_type = layer_type_from_csv_token(row.layer_type_token.value());
        if (!layer_type.has_value()) {
            std::vector<std::string> err_lines{};
            err_lines.push_back(format.format(
                "{}:{}: invalid layerType '{}'",
                FormatParam{path.string(), Style::bold},
                FormatParam{line_index + 1, Style::bold},
                FormatParam{row.layer_type_token.value(), Style::bold}));
            err_lines.emplace_back();
            err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
            return ChainableResult<void>{FormattableError{std::move(err_lines)}, layer_type};
        }
        attribute.explicit_layer_type(layer_type.value());
        return {};
    };

    for (std::size_t line_index = 1; line_index < lines.size(); ++line_index) {
        const auto &line = lines[line_index];

        if (line.empty()) {
            continue;
        }

        ChainableResult<CsvRow> row_result =
            csv_format == CsvFormat::firered
                ? parse_firered_csv_row(line, line_index, path, lines, format, file_printer, has_layer_type_column)
                : parse_emerald_csv_row(line, line_index, path, lines, format, file_printer, has_layer_type_column);

        if (!row_result.has_value()) {
            return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                FormattableError{"Failed to parse CSV row."}, row_result};
        }

        const auto &row = row_result.value();

        if (id_to_line_index.contains(row.metatile_id)) {
            const std::size_t original_line_index = id_to_line_index.at(row.metatile_id);
            std::vector<std::string> err_lines{};

            // Header for duplicate location
            err_lines.push_back(format.format(
                "{}:{}: duplicate metatile id '{}'",
                FormatParam{path.string(), Style::bold},
                FormatParam{line_index + 1, Style::bold},
                FormatParam{row.metatile_id, Style::bold}));
            err_lines.emplace_back();

            // File context for duplicate
            err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
            err_lines.emplace_back();

            // Note about original location
            err_lines.push_back(format.format(
                "{} originally defined at line {}:",
                FormatParam{"note:", Style::cyan | Style::bold},
                FormatParam{original_line_index + 1}));

            // File context for original
            err_lines.append_range(file_printer.print(lines, std::vector{original_line_index}));

            return FormattableError{std::move(err_lines)};
        }
        id_to_line_index.emplace(row.metatile_id, line_index);

        auto behavior_value = behavior_map.lookup(row.behavior);
        if (!behavior_value.has_value()) {
            std::vector<std::string> err_lines{};
            err_lines.push_back(format.format(
                "{}:{}: unknown metatile behavior '{}'",
                FormatParam{path.string(), Style::bold},
                FormatParam{line_index + 1, Style::bold},
                FormatParam{row.behavior, Style::bold}));
            err_lines.emplace_back();
            err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
            return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                FormattableError{std::move(err_lines)}, behavior_value};
        }

        if (csv_format == CsvFormat::firered) {
            // Resolve terrain type
            auto terrain_value = terrain_map->lookup(row.terrain_type);
            if (!terrain_value.has_value()) {
                std::vector<std::string> err_lines{};
                err_lines.push_back(format.format(
                    "{}:{}: unknown terrain type '{}'",
                    FormatParam{path.string(), Style::bold},
                    FormatParam{line_index + 1, Style::bold},
                    FormatParam{row.terrain_type, Style::bold}));
                err_lines.emplace_back();
                err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
                return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                    FormattableError{std::move(err_lines)}, terrain_value};
            }

            // Resolve encounter type
            auto encounter_value = encounter_map->lookup(row.encounter_type);
            if (!encounter_value.has_value()) {
                std::vector<std::string> err_lines{};
                err_lines.push_back(format.format(
                    "{}:{}: unknown encounter type '{}'",
                    FormatParam{path.string(), Style::bold},
                    FormatParam{line_index + 1, Style::bold},
                    FormatParam{row.encounter_type, Style::bold}));
                err_lines.emplace_back();
                err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
                return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                    FormattableError{std::move(err_lines)}, encounter_value};
            }

            MetatileAttribute attribute{};
            attribute.layer_type(LayerType::normal);
            attribute.field(attr::field_behavior, behavior_value.value());
            attribute.field(attr::field_terrain, terrain_value.value());
            attribute.field(attr::field_encounter_type, encounter_value.value());
            if (const auto applied = apply_explicit_layer_type(attribute, row, line_index); !applied.has_value()) {
                return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                    FormattableError{"Failed to apply layerType override."}, applied};
            }
            result.emplace(row.metatile_id, std::move(attribute));
        }
        else {
            MetatileAttribute attribute{};
            attribute.layer_type(LayerType::normal);
            attribute.field(attr::field_behavior, behavior_value.value());
            if (const auto applied = apply_explicit_layer_type(attribute, row, line_index); !applied.has_value()) {
                return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                    FormattableError{"Failed to apply layerType override."}, applied};
            }
            result.emplace(row.metatile_id, std::move(attribute));
        }
    }

    return result;
}

} // namespace

namespace porytiles {

ChainableResult<std::map<std::size_t, MetatileAttribute>>
AttributesCsvLoader::load(const std::filesystem::path &path, const std::string &tileset_name) const
{
    // Resolve the knob under the file's owning tileset scope. When compiling a secondary, the paired primary's CSV
    // loads through this same loader with the primary's name, so its knob resolves under the primary's config. (The
    // result type has a comma, so it cannot go through the PT_TRY_ASSIGN macros; unwrap by hand.)
    auto write_layer_type_column_cv = config_->write_layer_type_column(ConfigScopeType::tileset, tileset_name);
    if (!write_layer_type_column_cv.has_value()) {
        return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
            FormattableError{"Failed to resolve write_layer_type_column."}, write_layer_type_column_cv};
    }

    return parse_attributes_csv(
        path,
        *behavior_map_,
        base_game_,
        terrain_map_,
        encounter_map_,
        *format_,
        *file_printer_,
        write_layer_type_column_cv.value(),
        *diag_);
}

} // namespace porytiles

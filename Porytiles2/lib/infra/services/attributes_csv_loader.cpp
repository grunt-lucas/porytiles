#include "porytiles2/infra/services/attributes_csv_loader.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#include "porytiles2/domain/models/layer.hpp"
#include "porytiles2/utilities/parse_int.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

enum class CsvFormat { emerald, firered };

struct CsvRow {
    std::size_t metatile_id;
    std::string behavior;
    std::string terrain_type;
    std::string encounter_type;
};

ChainableResult<CsvRow> parse_emerald_csv_row(
    const std::string &line,
    std::size_t line_index,
    const std::filesystem::path &path,
    const std::vector<std::string> &all_lines,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer)
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
        std::ranges::copy(file_printer.print(all_lines, std::vector{line_index}), std::back_inserter(err_lines));
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
        std::ranges::copy(file_printer.print(all_lines, std::vector{line_index}), std::back_inserter(err_lines));
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
        std::ranges::copy(file_printer.print(all_lines, std::vector{line_index}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }

    return CsvRow{static_cast<std::size_t>(id_result.value()), columns[1], "", ""};
}

ChainableResult<CsvRow> parse_firered_csv_row(
    const std::string &line,
    std::size_t line_index,
    const std::filesystem::path &path,
    const std::vector<std::string> &all_lines,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer)
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
        std::ranges::copy(file_printer.print(all_lines, std::vector{line_index}), std::back_inserter(err_lines));
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
        std::ranges::copy(file_printer.print(all_lines, std::vector{line_index}), std::back_inserter(err_lines));
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
        std::ranges::copy(file_printer.print(all_lines, std::vector{line_index}), std::back_inserter(err_lines));
        return FormattableError{std::move(err_lines)};
    }

    return CsvRow{static_cast<std::size_t>(id_result.value()), columns[1], columns[2], columns[3]};
}

ChainableResult<std::map<std::size_t, MetatileAttribute>> parse_attributes_csv(
    const std::filesystem::path &path,
    const BehaviorMapProvider &behavior_map,
    std::optional<BaseGame> base_game,
    const TerrainTypeMapProvider *terrain_map,
    const EncounterTypeMapProvider *encounter_map,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer)
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
        std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{0}), std::back_inserter(err_lines));
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
            std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{0}), std::back_inserter(err_lines));
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
            std::ranges::copy(file_printer.print(lines, std::vector<std::size_t>{0}), std::back_inserter(err_lines));
            return FormattableError{std::move(err_lines)};
        }
    }

    // Validate provider availability for FireRed format
    if (csv_format == CsvFormat::firered) {
        if (terrain_map == nullptr) {
            panic("firered csv format requires a terrain type provider but none was provided");
        }
        if (encounter_map == nullptr) {
            panic("firered csv format requires an encounter type provider but none was provided");
        }
    }

    // Parse data rows (starting at index 1)
    std::map<std::size_t, MetatileAttribute> result{};
    std::unordered_map<std::size_t, std::size_t> id_to_line_index{};

    for (std::size_t line_index = 1; line_index < lines.size(); ++line_index) {
        const auto &line = lines[line_index];

        if (line.empty()) {
            continue;
        }

        ChainableResult<CsvRow> row_result =
            csv_format == CsvFormat::firered
                ? parse_firered_csv_row(line, line_index, path, lines, format, file_printer)
                : parse_emerald_csv_row(line, line_index, path, lines, format, file_printer);

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
            std::ranges::copy(file_printer.print(lines, std::vector{line_index}), std::back_inserter(err_lines));
            err_lines.emplace_back();

            // Note about original location
            err_lines.push_back(format.format(
                "{} originally defined at line {}:",
                FormatParam{"note:", Style::cyan | Style::bold},
                FormatParam{original_line_index + 1}));

            // File context for original
            std::ranges::copy(
                file_printer.print(lines, std::vector{original_line_index}), std::back_inserter(err_lines));

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
            std::ranges::copy(file_printer.print(lines, std::vector{line_index}), std::back_inserter(err_lines));
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
                std::ranges::copy(file_printer.print(lines, std::vector{line_index}), std::back_inserter(err_lines));
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
                std::ranges::copy(file_printer.print(lines, std::vector{line_index}), std::back_inserter(err_lines));
                return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                    FormattableError{std::move(err_lines)}, encounter_value};
            }

            result.emplace(
                row.metatile_id,
                MetatileAttribute{
                    LayerType::normal,
                    behavior_value.value(),
                    terrain_value.value(),
                    encounter_value.value(),
                    0,
                    0,
                    0,
                    false});
        }
        else {
            result.emplace(row.metatile_id, MetatileAttribute{LayerType::normal, behavior_value.value()});
        }
    }

    return result;
}

} // namespace

namespace porytiles2 {

ChainableResult<std::map<std::size_t, MetatileAttribute>>
AttributesCsvLoader::load(const std::filesystem::path &path) const
{
    return parse_attributes_csv(
        path, *behavior_map_, base_game_, terrain_map_, encounter_map_, *format_, *file_printer_);
}

} // namespace porytiles2

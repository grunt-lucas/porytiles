#include "porytiles/infra/services/attributes_csv_loader.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "porytiles/domain/models/layer.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/parse_int.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace {

using namespace porytiles;

struct CsvRow {
    std::size_t metatile_id;
    std::vector<std::string> field_cells;        // one trimmed cell per schema field, in schema order
    std::optional<std::string> layer_type_token; // raw layer_type cell, nullopt when the column or cell is blank
};

/**
 * @brief Renders the header row the schema expects: id plus every field name in schema order.
 */
[[nodiscard]] std::string expected_header_string(const Schema &schema)
{
    std::string header = "id";
    for (const Field &field : schema.fields()) {
        header += "," + field.name();
    }
    return header;
}

/**
 * @brief Extracts the trimmed layer_type cell at a fixed column index, or nullopt when the cell is blank/absent.
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

/**
 * @brief Splits one data row into an id, one cell per schema field, and the optional layer_type token.
 *
 * @details
 * The cells are not resolved here; the caller interprets each one against its schema field (provider lookup or raw
 * integer parse). This keeps row shape errors (too few columns, bad id) separate from value errors.
 */
ChainableResult<CsvRow> parse_csv_row(
    const std::string &line,
    std::size_t line_index,
    const std::filesystem::path &path,
    const std::vector<std::string> &all_lines,
    const Schema &schema,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer,
    bool has_layer_type_column)
{
    const std::size_t field_count = schema.fields().size();
    auto columns = split(line, ",");

    if (columns.size() < 1 + field_count) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected at least {} columns ({}), found {}",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{1 + field_count},
            FormatParam{expected_header_string(schema)},
            FormatParam{columns.size()}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    // The row-level mirror of the header's unexpected-column check: a data row wider than the header shape means the
    // CSV was written for a wider schema, and its extra cells must fail loudly instead of being silently dropped.
    const std::size_t max_columns = 1 + field_count + (has_layer_type_column ? 1 : 0);
    if (columns.size() > max_columns) {
        std::vector<std::string> err_lines{};
        err_lines.push_back(format.format(
            "{}:{}: expected at most {} columns ({}{}), found {}",
            FormatParam{path.string(), Style::bold},
            FormatParam{line_index + 1, Style::bold},
            FormatParam{max_columns},
            FormatParam{expected_header_string(schema)},
            FormatParam{has_layer_type_column ? ",layer_type" : ""},
            FormatParam{columns.size()}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(all_lines, std::vector{line_index}));
        return FormattableError{std::move(err_lines)};
    }

    for (std::size_t i = 0; i <= field_count; ++i) {
        trim(columns[i]);
    }

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

    std::vector<std::string> field_cells{};
    field_cells.reserve(field_count);
    for (std::size_t i = 0; i < field_count; ++i) {
        field_cells.push_back(columns[1 + i]);
    }

    // The layer_type column, when present, sits directly after the schema fields.
    return CsvRow{
        static_cast<std::size_t>(id_result.value()),
        std::move(field_cells),
        extract_layer_type_token(columns, has_layer_type_column, 1 + field_count)};
}

ChainableResult<std::map<std::size_t, MetatileAttribute>> parse_attributes_csv(
    const std::filesystem::path &path,
    const Schema &schema,
    const ProviderMap &providers,
    const TextFormatter &format,
    const FileHighlightPrinter &file_printer,
    bool write_layer_type_column,
    const UserDiagnostics &diag)
{
    if (!exists(path)) {
        return FormattableError{"{}: file does not exist.", FormatParam{path.string(), Style::bold}};
    }

    const std::string expected_header = expected_header_string(schema);

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
            "{}: file is empty, expected header '{}'",
            FormatParam{path.string(), Style::bold},
            FormatParam{expected_header, Style::bold}};
    }

    // Cross-check the header line (index 0) against the resolved schema: the columns must be 'id' followed by every
    // schema field name in schema order, then at most an optional trailing layer_type column. Anything missing,
    // mis-ordered, or extra is a schema mismatch and fails with a diagnostic naming the column and its position.
    auto header_columns = split(lines[0], ",");
    for (auto &col : header_columns) {
        trim(col);
    }

    const std::size_t field_count = schema.fields().size();
    auto make_header_error = [&](const std::string &message) -> FormattableError {
        std::vector<std::string> err_lines{};
        err_lines.push_back(message);
        err_lines.push_back(format.format(
            "expected header (from the resolved attribute schema): '{}'", FormatParam{expected_header, Style::bold}));
        err_lines.emplace_back();
        err_lines.append_range(file_printer.print(lines, std::vector<std::size_t>{0}));
        return FormattableError{std::move(err_lines)};
    };

    for (std::size_t i = 0; i <= field_count; ++i) {
        const std::string &expected_column = i == 0 ? "id" : schema.fields()[i - 1].name();
        if (i >= header_columns.size()) {
            return make_header_error(format.format(
                "{}:{}: invalid header: missing column '{}' at position {}",
                FormatParam{path.string(), Style::bold},
                FormatParam{"1", Style::bold},
                FormatParam{expected_column, Style::bold},
                FormatParam{i + 1}));
        }
        if (header_columns[i] != expected_column) {
            return make_header_error(format.format(
                "{}:{}: invalid header: expected column {} to be '{}' but found '{}'",
                FormatParam{path.string(), Style::bold},
                FormatParam{"1", Style::bold},
                FormatParam{i + 1},
                FormatParam{expected_column, Style::bold},
                FormatParam{header_columns[i], Style::bold}));
        }
    }

    // Detect an optional trailing layer_type column directly after the schema fields. We always detect it; the knob
    // decides whether its values are applied.
    const std::size_t layer_type_index = 1 + field_count;
    const bool has_layer_type_column =
        header_columns.size() > layer_type_index && header_columns[layer_type_index] == "layer_type";

    // Any trailing column other than the optional layer_type is a schema mismatch, not a tolerated extra: a CSV written
    // for a wider schema (more fields than this tileset resolves) must fail loudly instead of silently dropping its
    // extra fields.
    const std::size_t first_unexpected_index = layer_type_index + (has_layer_type_column ? 1 : 0);
    if (header_columns.size() > first_unexpected_index) {
        return make_header_error(format.format(
            "{}:{}: invalid header: unexpected column '{}' at position {}",
            FormatParam{path.string(), Style::bold},
            FormatParam{"1", Style::bold},
            FormatParam{header_columns[first_unexpected_index], Style::bold},
            FormatParam{first_unexpected_index + 1}));
    }

    // Knob off but the column is present: ignore the values and say so once for the whole file.
    if (has_layer_type_column && !write_layer_type_column) {
        diag.warning(
            "layer-type-column",
            std::vector<std::string>{
                format.format(
                    "{}: a layer_type column is present but write_layer_type_column is off; its values are ignored and "
                    "layer types will be inferred.",
                    FormatParam{path.string(), Style::bold}),
                format.format(
                    "set {} to apply the column.",
                    FormatParam{"fieldmap.write_layer_type_column: true", Style::bold})});
    }

    // Parse data rows (starting at index 1)
    std::map<std::size_t, MetatileAttribute> result{};
    std::unordered_map<std::size_t, std::size_t> id_to_line_index{};

    // Applies a filled layer_type cell as an explicit override, when the column is present and the knob is on. A blank
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
                "{}:{}: invalid layer_type '{}'",
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

    // Resolves one field cell to its numeric value. A provider-backed field goes through its provider (the ProviderMap
    // membership contract makes a missing provider an internal bug, not a raw fallback); a raw field parses as an
    // unsigned integer capped at the field's maximum, which the binary writer would otherwise silently mask away.
    auto resolve_field_cell =
        [&](const Field &field, const std::string &cell, std::size_t line_index) -> ChainableResult<std::uint32_t> {
        if (field.has_provider()) {
            const auto provider_it = providers.find(field.name());
            if (provider_it == providers.end()) {
                panic(
                    std::format(
                        "parse_attributes_csv: field '{}' has a provider spec but no provider was built for it",
                        field.name()));
            }
            auto lookup_result = provider_it->second->lookup(cell);
            if (!lookup_result.has_value()) {
                std::vector<std::string> err_lines{};
                err_lines.push_back(format.format(
                    "{}:{}: unknown {} '{}'",
                    FormatParam{path.string(), Style::bold},
                    FormatParam{line_index + 1, Style::bold},
                    FormatParam{field.name()},
                    FormatParam{cell, Style::bold}));
                err_lines.emplace_back();
                err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
                return ChainableResult<std::uint32_t>{FormattableError{std::move(err_lines)}, lookup_result};
            }
            return lookup_result.value();
        }

        auto int_result = parse_int<long long>(cell, 0);
        if (!int_result.has_value() || int_result.value() < 0) {
            std::vector<std::string> err_lines{};
            err_lines.push_back(format.format(
                "{}:{}: invalid {} value '{}': expected an unsigned integer",
                FormatParam{path.string(), Style::bold},
                FormatParam{line_index + 1, Style::bold},
                FormatParam{field.name()},
                FormatParam{cell, Style::bold}));
            err_lines.emplace_back();
            err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
            return FormattableError{std::move(err_lines)};
        }
        if (int_result.value() > static_cast<long long>(field.max_value())) {
            std::vector<std::string> err_lines{};
            err_lines.push_back(format.format(
                "{}:{}: {} value '{}' exceeds the field's maximum of {}",
                FormatParam{path.string(), Style::bold},
                FormatParam{line_index + 1, Style::bold},
                FormatParam{field.name()},
                FormatParam{cell, Style::bold},
                FormatParam{field.max_value(), Style::bold}));
            err_lines.emplace_back();
            err_lines.append_range(file_printer.print(lines, std::vector{line_index}));
            return FormattableError{std::move(err_lines)};
        }
        return static_cast<std::uint32_t>(int_result.value());
    };

    for (std::size_t line_index = 1; line_index < lines.size(); ++line_index) {
        const auto &line = lines[line_index];

        if (line.empty()) {
            continue;
        }

        ChainableResult<CsvRow> row_result =
            parse_csv_row(line, line_index, path, lines, schema, format, file_printer, has_layer_type_column);

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

        MetatileAttribute attribute{};
        attribute.layer_type(LayerType::normal);
        for (std::size_t i = 0; i < field_count; ++i) {
            const Field &field = schema.fields()[i];
            auto value_result = resolve_field_cell(field, row.field_cells[i], line_index);
            if (!value_result.has_value()) {
                return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                    FormattableError{"Failed to resolve CSV field cell."}, value_result};
            }
            attribute.field(field.name(), value_result.value());
        }
        if (const auto applied = apply_explicit_layer_type(attribute, row, line_index); !applied.has_value()) {
            return ChainableResult<std::map<std::size_t, MetatileAttribute>>{
                FormattableError{"Failed to apply layer_type override."}, applied};
        }
        result.emplace(row.metatile_id, std::move(attribute));
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
        path, *schema_, *providers_, *format_, *file_printer_, write_layer_type_column_cv.value(), *diag_);
}

} // namespace porytiles

#include "porytiles/utilities/c_parser/c_parser_facade.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "porytiles/utilities/c_parser/c_parser_context.hpp"
#include "porytiles/utilities/c_parser/lexer.hpp"
#include "porytiles/utilities/c_parser/parser.hpp"
#include "porytiles/utilities/string_utils.hpp"

namespace porytiles {

namespace {

[[nodiscard]] std::unordered_map<std::string, std::int64_t> resolved_define_values(const TolerantDefineScan &scan)
{
    std::unordered_map<std::string, std::int64_t> values;
    for (const DefineStatement &define : scan.defines) {
        if (define.has_int_value()) {
            values.insert_or_assign(define.name(), define.int_value());
        }
    }
    return values;
}

[[nodiscard]] ChainableResult<TolerantDefineScan> scan_define_values_for_enums(
    gsl::not_null<const TextFormatter *> format,
    const std::string &content,
    const CParserContext *context,
    const std::unordered_map<std::string, std::int64_t> &seed_symbols)
{
    Lexer lexer{format, content, context};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<TolerantDefineScan>{lex_result};
    }

    Parser parser{format, std::move(lex_result).value(), context};
    parser.seed_symbols(seed_symbols);
    TolerantDefineScan scan = parser.parse_defines_tolerant();
    scan.ambiguous_values = parser.ambiguous_defines();
    return scan;
}

} // namespace

CParserFacade::CParserFacade(std::filesystem::path file_path, gsl::not_null<const TextFormatter *> format)
    : file_path_{std::move(file_path)}, format_{format}
{
}

CParserFacade::CParserFacade(
    std::filesystem::path file_path,
    gsl::not_null<const TextFormatter *> format,
    std::unordered_map<std::string, std::int64_t> seed_symbols)
    : file_path_{std::move(file_path)}, format_{format}, seed_symbols_{std::move(seed_symbols)}
{
}

Parser CParserFacade::make_seeded_parser(std::vector<Token> tokens)
{
    Parser parser{format_, std::move(tokens), context_.get()};
    if (!seed_symbols_.empty()) {
        parser.seed_symbols(seed_symbols_);
    }
    return parser;
}

ChainableResult<void> CParserFacade::ensure_loaded()
{
    if (loaded_) {
        return {};
    }

    if (load_failed_) {
        return load_error_;
    }

    // Attempt to load the file
    if (!std::filesystem::exists(file_path_)) {
        load_failed_ = true;
        load_error_ =
            FormattableError{format_->format("{}: file not found", FormatParam{file_path_.string(), Style::bold})};
        return load_error_;
    }

    std::ifstream file{file_path_};
    if (!file.is_open()) {
        load_failed_ = true;
        load_error_ =
            FormattableError{format_->format("{}: failed to open file", FormatParam{file_path_.string(), Style::bold})};
        return load_error_;
    }

    // Read entire file content
    std::ostringstream buffer;
    buffer << file.rdbuf();
    content_ = buffer.str();

    // Split content into lines for FileHighlightPrinter
    file_lines_.clear();
    std::istringstream line_stream{content_};
    std::string line;
    while (std::getline(line_stream, line)) {
        line = trim_line_ending(line);
        file_lines_.push_back(line);
    }

    // Create context for rich error formatting
    context_ = std::make_unique<CParserContext>(&file_lines_, format_, file_path_.string());

    loaded_ = true;
    return {};
}

ChainableResult<std::vector<DefineStatement>> CParserFacade::parse_defines()
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<DefineStatement>>{
            FormattableError{format_->format(
                "{}: failed to parse #define statements", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<DefineStatement>>{
            FormattableError{format_->format(
                "{}: failed to parse #define statements", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_defines();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<DefineStatement>>{
            FormattableError{format_->format(
                "{}: failed to parse #define statements", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    return std::move(parse_result).value();
}

ChainableResult<std::vector<EnumDeclaration>> CParserFacade::parse_enums()
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<EnumDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Enum values commonly refer to integer #defines in the same header. Use a tolerant define scan so unrelated
    // function-like or otherwise unevaluable macros do not prevent the enum parser from receiving the values it needs.
    auto defines_result = scan_define_values_for_enums(format_, content_, context_.get(), seed_symbols_);
    if (!defines_result.has_value()) {
        return ChainableResult<std::vector<EnumDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            defines_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<EnumDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    parser.seed_values(resolved_define_values(defines_result.value()));
    auto parse_result = parser.parse_enums();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<EnumDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    return std::move(parse_result).value();
}

ChainableResult<TolerantDefineScan> CParserFacade::parse_defines_tolerant()
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<TolerantDefineScan>{
            FormattableError{format_->format(
                "{}: failed to parse #define statements", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<TolerantDefineScan>{
            FormattableError{format_->format(
                "{}: failed to parse #define statements", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    auto parser = make_seeded_parser(std::move(lex_result).value());
    TolerantDefineScan scan = parser.parse_defines_tolerant();
    scan.ambiguous_values = parser.ambiguous_defines();
    scan_warnings_.insert(scan_warnings_.end(), parser.scan_warnings().begin(), parser.scan_warnings().end());
    return scan;
}

ChainableResult<TolerantEnumScan> CParserFacade::parse_enums_tolerant()
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<TolerantEnumScan>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    auto defines_result = scan_define_values_for_enums(format_, content_, context_.get(), seed_symbols_);
    if (!defines_result.has_value()) {
        return ChainableResult<TolerantEnumScan>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            defines_result};
    }

    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<TolerantEnumScan>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    auto parser = make_seeded_parser(std::move(lex_result).value());
    parser.seed_values(resolved_define_values(defines_result.value()));
    TolerantEnumScan scan = parser.parse_enums_tolerant();
    scan_warnings_.insert(scan_warnings_.end(), parser.scan_warnings().begin(), parser.scan_warnings().end());
    return scan;
}

ChainableResult<std::vector<ArrayDeclaration>>
CParserFacade::parse_pointer_arrays(const std::optional<std::string> &name_prefix)
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<ArrayDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse pointer arrays", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<ArrayDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse pointer arrays", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_pointer_arrays();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<ArrayDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse pointer arrays", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    // Apply name prefix filter if provided
    auto arrays = std::move(parse_result).value();
    if (name_prefix.has_value()) {
        std::erase_if(
            arrays, [&](const ArrayDeclaration &arr) { return !arr.name().starts_with(name_prefix.value()); });
    }

    return arrays;
}

ChainableResult<std::vector<FunctionDefinition>>
CParserFacade::parse_functions(const std::optional<std::string> &name_prefix)
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<FunctionDefinition>>{
            FormattableError{
                format_->format("{}: failed to parse functions", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<FunctionDefinition>>{
            FormattableError{
                format_->format("{}: failed to parse functions", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_functions();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<FunctionDefinition>>{
            FormattableError{
                format_->format("{}: failed to parse functions", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    // Apply name prefix filter if provided
    auto functions = std::move(parse_result).value();
    if (name_prefix.has_value()) {
        std::erase_if(
            functions, [&](const FunctionDefinition &func) { return !func.name().starts_with(name_prefix.value()); });
    }

    return functions;
}

ChainableResult<std::vector<StructVariableDeclaration>>
CParserFacade::parse_struct_variables(const std::optional<std::string> &name_prefix)
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<StructVariableDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse struct variables", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<StructVariableDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse struct variables", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_struct_variables();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<StructVariableDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse struct variables", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    // Apply name prefix filter if provided
    auto structs = std::move(parse_result).value();
    if (name_prefix.has_value()) {
        std::erase_if(structs, [&](const StructVariableDeclaration &s) {
            return !s.variable_name().starts_with(name_prefix.value());
        });
    }

    return structs;
}

ChainableResult<std::vector<StructInitializerDeclaration>>
CParserFacade::parse_struct_initializers(const std::optional<std::string> &name_prefix)
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<StructInitializerDeclaration>>{
            FormattableError{format_->format(
                "{}: failed to parse struct initializers", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<StructInitializerDeclaration>>{
            FormattableError{format_->format(
                "{}: failed to parse struct initializers", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_struct_initializers();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<StructInitializerDeclaration>>{
            FormattableError{format_->format(
                "{}: failed to parse struct initializers", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    // Apply name prefix filter if provided
    auto structs = std::move(parse_result).value();
    if (name_prefix.has_value()) {
        std::erase_if(structs, [&](const StructInitializerDeclaration &s) {
            return !s.variable_name().starts_with(name_prefix.value());
        });
    }

    return structs;
}

ChainableResult<std::vector<StructDefinition>>
CParserFacade::parse_struct_definitions(const std::optional<std::string> &name_filter)
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<StructDefinition>>{
            FormattableError{format_->format(
                "{}: failed to parse struct definitions", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<StructDefinition>>{
            FormattableError{format_->format(
                "{}: failed to parse struct definitions", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_struct_definitions();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<StructDefinition>>{
            FormattableError{format_->format(
                "{}: failed to parse struct definitions", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    // Apply exact-name filter if provided
    auto definitions = std::move(parse_result).value();
    if (name_filter.has_value()) {
        std::erase_if(definitions, [&](const StructDefinition &def) { return def.name != name_filter.value(); });
    }

    return definitions;
}

ChainableResult<std::vector<IncbinDeclaration>>
CParserFacade::parse_incbin_arrays(const std::optional<std::string> &name_prefix)
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<IncbinDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse INCBIN arrays", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    // Lex the content
    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<IncbinDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse INCBIN arrays", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    // Parse the tokens
    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_incbin_arrays();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<IncbinDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse INCBIN arrays", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    // Apply name prefix filter if provided
    auto incbins = std::move(parse_result).value();
    if (name_prefix.has_value()) {
        std::erase_if(incbins, [&](const IncbinDeclaration &inc) {
            return !inc.variable_name().starts_with(name_prefix.value());
        });
    }

    return incbins;
}

ChainableResult<std::vector<IndexedArrayDeclaration>>
CParserFacade::parse_indexed_arrays(const std::optional<std::string> &name_prefix)
{
    auto load_result = ensure_loaded();
    if (!load_result.has_value()) {
        return ChainableResult<std::vector<IndexedArrayDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse indexed arrays", FormatParam{file_path_.string(), Style::bold})},
            load_result};
    }

    Lexer lexer{format_, content_, context_.get()};
    auto lex_result = lexer.lex();
    if (!lex_result.has_value()) {
        return ChainableResult<std::vector<IndexedArrayDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse indexed arrays", FormatParam{file_path_.string(), Style::bold})},
            lex_result};
    }

    auto parser = make_seeded_parser(std::move(lex_result).value());
    auto parse_result = parser.parse_indexed_arrays();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<IndexedArrayDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse indexed arrays", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    // Apply name prefix filter if provided
    auto arrays = std::move(parse_result).value();
    if (name_prefix.has_value()) {
        std::erase_if(
            arrays, [&](const IndexedArrayDeclaration &arr) { return !arr.name.starts_with(name_prefix.value()); });
    }

    return arrays;
}

ChainableResult<std::optional<DefineStatement>> CParserFacade::find_define(const std::string &define_name)
{
    // Ensure defines are cached
    if (!cached_defines_.has_value()) {
        auto result = parse_defines();
        if (!result.has_value()) {
            return ChainableResult<std::optional<DefineStatement>>{
                FormattableError{format_->format(
                    "{}: failed to find #define '{}'",
                    FormatParam{file_path_.string(), Style::bold},
                    FormatParam{define_name, Style::bold})},
                result};
        }
        cached_defines_ = std::move(result.value());
    }

    // Search for the define
    for (const auto &define : cached_defines_.value()) {
        if (define.name() == define_name) {
            return std::optional{define};
        }
    }

    // Not found - this is not an error, just return nullopt
    return std::optional<DefineStatement>{std::nullopt};
}

const std::vector<std::string> &CParserFacade::file_lines() const
{
    return file_lines_;
}

} // namespace porytiles

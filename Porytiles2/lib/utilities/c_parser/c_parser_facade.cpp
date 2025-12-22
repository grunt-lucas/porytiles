#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "porytiles2/utilities/c_parser/c_parser_context.hpp"
#include "porytiles2/utilities/c_parser/lexer.hpp"
#include "porytiles2/utilities/c_parser/parser.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace porytiles2 {

CParserFacade::CParserFacade(std::filesystem::path file_path, gsl::not_null<const TextFormatter *> format)
    : file_path_{std::move(file_path)}, format_{format}
{
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
    Parser parser{format_, std::move(lex_result).value(), context_.get()};
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
    Parser parser{format_, std::move(lex_result).value(), context_.get()};
    auto parse_result = parser.parse_enums();
    if (!parse_result.has_value()) {
        return ChainableResult<std::vector<EnumDeclaration>>{
            FormattableError{
                format_->format("{}: failed to parse enums", FormatParam{file_path_.string(), Style::bold})},
            parse_result};
    }

    return std::move(parse_result).value();
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
    Parser parser{format_, std::move(lex_result).value(), context_.get()};
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
    Parser parser{format_, std::move(lex_result).value(), context_.get()};
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

} // namespace porytiles2

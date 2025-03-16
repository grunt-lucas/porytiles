#ifndef PORYTILES_UTILITIES_H
#define PORYTILES_UTILITIES_H

#include <filesystem>
#include <string>

#include "./errors_warnings.h"
#include "./porytiles_context.h"
#include "./types.h"

namespace porytiles {

template <typename T> T parseInteger(const char *integerString, const int base) {
    try {
        std::size_t pos;
        T arg = std::stoi(integerString, &pos, base);
        if (std::string{integerString}.size() != pos) {
            // throw here so it catches below and prints an error message
            throw std::runtime_error{"invalid integral string: " + std::string{integerString}};
        }
        return arg;
    } catch (const std::exception &e) {
        throw std::runtime_error{e.what()};
    }
    // unreachable, here for compiler
    throw std::runtime_error("utilities::parseInteger reached unreachable code path");
}

template <typename T> T parseInteger(const char *integerString) {
    return parseInteger<T>(integerString, 0);
}

std::vector<std::string> split(std::string input, const std::string &delimiter);

bool checkFullStringMatch(const std::string &str, const std::string &pattern);

void trim(std::string &string);

std::filesystem::path getTmpfilePath(const std::filesystem::path &parentDir, const std::string &fileName);

std::filesystem::path createTmpdir();

RGBA32 parseJascLineCompiler(PorytilesContext &ctx, CompilerMode compilerMode, const std::string &jascLine,
                             const std::string &fileName);

RGBA32 parseJascLineDecompiler(PorytilesContext &ctx, DecompilerMode decompilerMode, const std::string &jascLine,
                               const std::string &fileName);

void doctestAssertFileBytesIdentical(std::filesystem::path expectedPath, std::filesystem::path actualPath);

void doctestAssertFileLinesIdentical(std::filesystem::path expectedPath, std::filesystem::path actualPath);

} // namespace porytiles

#endif // PORYTILES_UTILITIES_H

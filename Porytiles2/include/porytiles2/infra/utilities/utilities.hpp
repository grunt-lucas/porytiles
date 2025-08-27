#ifndef PORYTILES_UTILITIES_H
#define PORYTILES_UTILITIES_H

#include <filesystem>
#include <string>
#include <vector>

namespace porytiles2 {

template <typename T>
T parse_integer(const char *integerString, const int base)
{
    try {
        std::size_t pos;
        T arg = std::stoi(integerString, &pos, base);
        if (std::string{integerString}.size() != pos) {
            // throw here so it catches below and prints an error message
            throw std::runtime_error{"invalid integral string: " + std::string{integerString}};
        }
        return arg;
    }
    catch (const std::exception &e) {
        throw std::runtime_error{e.what()};
    }
    // unreachable, here for compiler
    throw std::runtime_error("utilities::parseInteger reached unreachable code path");
}

template <typename T>
T parse_integer(const char *integerString)
{
    return parse_integer<T>(integerString, 0);
}

std::vector<std::string> split(std::string input, const std::string &delimiter);

bool check_full_string_match(const std::string &str, const std::string &pattern);

void trim(std::string &string);

std::filesystem::path get_tmpfile_path(const std::filesystem::path &parentDir, const std::string &fileName);

std::filesystem::path create_tmpdir();

std::string pal_index_to_file_name(std::size_t index);

} // namespace porytiles2

#endif // PORYTILES_UTILITIES_H

#include "porytiles2/infra/utilities/utilities.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

std::filesystem::path get_tmpfile_path(const std::filesystem::path &parentDir, const std::string &fileName)
{
    return std::filesystem::temp_directory_path() / parentDir / fileName;
}

std::filesystem::path create_tmpdir()
{
    int maxTries = 1000;
    auto tmpDir = std::filesystem::temp_directory_path();
    int i = 0;
    std::random_device randomDevice;
    std::mt19937 mersennePrng(randomDevice());
    std::uniform_int_distribution<uint64_t> uniformIntDistribution(0);
    std::filesystem::path path;
    while (true) {
        std::stringstream stringStream;
        stringStream << std::hex << uniformIntDistribution(mersennePrng);
        path = tmpDir / ("porytiles_" + stringStream.str());
        if (std::filesystem::create_directory(path)) {
            break;
        }
        if (i == maxTries) {
            panic("tmpfiles::createTmpdir getTmpdirPath took too many tries");
        }
        i++;
    }
    return path;
}

std::string pal_index_to_file_name(std::size_t index)
{
    std::string file = std::to_string(index) + ".png";
    if (index < 10) {
        file = "0" + file;
    }
    return file;
}

} // namespace porytiles2

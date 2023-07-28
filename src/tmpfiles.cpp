#include "tmpfiles.h"

#include <random>
#include <sstream>
#include <filesystem>
#include <string>

namespace porytiles {

std::filesystem::path getTmpfilePath(const std::filesystem::path& parentDir, const std::string& fileName) {
    return std::filesystem::temp_directory_path() / parentDir / fileName;
}

std::filesystem::path createTmpdir() {
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
            throw std::runtime_error("getTmpdirPath took too many tries");
        }
        i++;
    }
    return path;
}

}
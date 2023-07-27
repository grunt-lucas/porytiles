#include "tmpfiles.h"

#include <random>
#include <sstream>
#include <filesystem>
#include <string>

namespace porytiles {

std::filesystem::path getTmpfilePath(std::string fileName) {
    return std::filesystem::temp_directory_path() / fileName;
}

std::filesystem::path getTmpdirPath() {
    // TODO : cleanup
    int maxTries = 1000;
    auto tmp_dir = std::filesystem::temp_directory_path();
    int i = 0;
    std::random_device dev;
    std::mt19937 prng(dev());
    std::uniform_int_distribution<uint64_t> rand(0);
    std::filesystem::path path;
    while (true) {
        std::stringstream ss;
        ss << std::hex << rand(prng);
        path = tmp_dir / ss.str();
        if (std::filesystem::create_directory(path)) {
            break;
        }
        if (i == maxTries) {
            throw std::runtime_error("could not find non-existing directory");
        }
        i++;
    }
    return path;
}

}
#include "tmpfiles.h"

#include <filesystem>
#include <string>

namespace porytiles {

std::filesystem::path getTmpfilePath(std::string fileName) {
    return std::filesystem::temp_directory_path() / fileName;
}

}
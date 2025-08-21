#pragma once

#include <string>

namespace porytiles2 {

struct ImageLoadError {
    enum class Type { file_not_found, unsupported_channel_count, other_load_error };
    Type type;
    std::string metadata;
};

} // namespace porytiles2

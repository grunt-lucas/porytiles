#pragma once

#include <memory>
#include <optional>

#include "porytiles2/domain/config/config.hpp"

namespace porytiles2 {

class ConfigRepo {
public:
    virtual ~ConfigRepo() = default;
    
    /**
     * @brief Loads a copy of the Config.
     */
    [[nodiscard]] virtual Config load() const = 0;
    
    virtual void save(const Config& config) const = 0;
};

} // namespace porytiles2

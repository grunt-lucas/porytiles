#pragma once

#include <string>
#include <unordered_map>

#include "porytiles2/templates/Result.hpp"

namespace porytiles {
class ChecksumService {
public:
  virtual ~ChecksumService() = default;

  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  ComputePorymapChecksums(const std::string &tileset_name) const = 0;

  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  LoadStoredChecksums(const std::string &tileset_name) const = 0;

  [[nodiscard]] virtual Result<void>
  StoreChecksums(const std::string &tileset_name,
                 const std::unordered_map<std::string, std::string> &checksums) = 0;
};

} // namespace porytiles

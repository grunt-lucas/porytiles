#pragma once

#include <string>
#include <unordered_map>

#include "porytiles2/domain/model/aggregates/Tileset.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {
class ChecksumService {
public:
  virtual ~ChecksumService() = default;

  /**
   * @brief Computes checksums for the artifacts that correspond to the given Tileset's
   * PorymapTilesetComponent.
   *
   * @details
   * TODO : fill in
   *
   * @param tileset The Tileset for which to compute checksums.
   * @return A mapping of artifact identifiers to their computed checksum.
   */
  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  ComputePorymapChecksums(const Tileset &tileset) const = 0;

  [[nodiscard]] virtual std::unordered_map<std::string, std::string>
  LoadStoredChecksums(const std::string &tileset_name) const = 0;

  [[nodiscard]] virtual Result<void>
  StoreChecksums(const std::string &tileset_name,
                 const std::unordered_map<std::string, std::string> &checksums) = 0;
};

} // namespace porytiles

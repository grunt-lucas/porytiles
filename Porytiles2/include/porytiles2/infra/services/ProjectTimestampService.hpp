// Porytiles2/include/porytiles2/infra/services/FilesystemTimestampService.hpp
#pragma once

#include "porytiles2/domain/services/TimestampService.hpp"

namespace porytiles {

/**
 * @brief Implementation of TimestampService that uses an in-filesystem `pokeemerald`
 * project as the source for artifact timestamp metadata.
 */
class ProjectTimestampService final : public TimestampService {
public:
  ProjectTimestampService() = default;

  [[nodiscard]] std::unordered_map<std::string, Timestamp>
  GetPorymapTimestamps(const Tileset &tileset) const override;

  [[nodiscard]] std::unordered_map<std::string, Timestamp>
  GetPorytilesTimestamps(const Tileset &tileset) const override;

  [[nodiscard]] bool ArePorymapAssetsNewer(const Tileset &tileset) const override;
};

} // namespace porytiles

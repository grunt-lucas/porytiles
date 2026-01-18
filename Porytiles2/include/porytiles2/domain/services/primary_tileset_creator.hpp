#pragma once

#include <memory>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Domain service for creating new primary tilesets from scratch.
 *
 * @details
 * This service creates a sample PorytilesTilesetComponent with some very basic layer art and animations. The
 * resulting component is ready to be compiled by PrimaryTilesetCompiler to produce minimal valid Porymap assets. Unlike
 * import workflows that read existing assets, this creates a tileset from nothing.
 *
 * This is a concrete class (not a virtual interface) since all creation logic uses pure domain objects without any
 * I/O dependencies.
 */
class PrimaryTilesetCreator {
  public:
    /**
     * @brief Constructs a PrimaryTilesetCreator with the given dependencies.
     *
     * @param config Domain configuration for tileset creation parameters
     * @param behavior_map The behavior map provider for resolving behavior names to values
     */
    PrimaryTilesetCreator(
        gsl::not_null<const DomainConfig *> config, gsl::not_null<const BehaviorMapProvider *> behavior_map)
        : config_{config}, behavior_map_{behavior_map}
    {
    }

    /**
     * @brief Creates a new basic PorytilesTilesetComponent with some default assets.
     *
     * @param tileset_name The name of the tileset being created (for error messages)
     * @return A new PorytilesTilesetComponent ready for compilation
     */
    [[nodiscard]] ChainableResult<std::unique_ptr<PorytilesTilesetComponent>>
    create_sample_porytiles_component(const std::string &tileset_name) const;

  private:
    const DomainConfig *config_;
    const BehaviorMapProvider *behavior_map_;
};

} // namespace porytiles2

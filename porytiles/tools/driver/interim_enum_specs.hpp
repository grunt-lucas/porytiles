#pragma once

#include <cstdint>
#include <limits>
#include <string>

#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"

namespace porytiles {

/*
 * Interim hardcoded EnumSpecs for the stock emerald/firered attribute layouts. Every driver command builds its header
 * enum providers from these while consumers still fork on base game. Once the schema walk issue is implemented (#284),
 * providers are built from each field's ProviderSpec via ProviderSpec::to_enum_spec and this file is deleted. The
 * display names are the #280 attr:: schema field names so the diagnostics already match the schema-driven future and
 * never change again in #284.
 */

/**
 * @brief The stock behavior field spec (prefix "MB_", declared as defines or an enum).
 */
[[nodiscard]] inline EnumSpec behavior_enum_spec()
{
    return EnumSpec{
        .prefix = "MB_",
        .max_value = std::numeric_limits<std::uint16_t>::max(),
        .skipped = {},
        .format = HeaderFormat::either,
        .field_display_name = std::string{attr::field_behavior}};
}

/**
 * @brief The stock terrain-type field spec (prefix "TILE_TERRAIN_", 5-bit values, enum-declared).
 */
[[nodiscard]] inline EnumSpec terrain_enum_spec()
{
    return EnumSpec{
        .prefix = "TILE_TERRAIN_",
        .max_value = 0x1F,
        .skipped = {},
        .format = HeaderFormat::enums_only,
        .field_display_name = std::string{attr::field_terrain}};
}

/**
 * @brief The stock encounter-type field spec (prefix "TILE_ENCOUNTER_", 3-bit values, enum-declared).
 */
[[nodiscard]] inline EnumSpec encounter_enum_spec()
{
    return EnumSpec{
        .prefix = "TILE_ENCOUNTER_",
        .max_value = 0x07,
        .skipped = {},
        .format = HeaderFormat::enums_only,
        .field_display_name = std::string{attr::field_encounter_type}};
}

} // namespace porytiles

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief Abstract interface for providing two-way name/value mappings for an enumerated attribute field.
///
/// @details
/// The base game names the values of a metatile attribute field (behavior constants, terrain types, encounter types,
/// and so on) through C header constants. An EnumMapProvider maps those constant names (e.g. "MB_NORMAL",
/// "TILE_TERRAIN_GRASS") to their numeric values and vice versa. A single field is one provider; the field a provider
/// serves is a construction-time detail, not a compile-time type. Implementations may load these mappings from various
/// sources such as header files or configuration, and loading and caching strategies are implementation details left to
/// concrete implementations.
///
/// Values are widened to uint32_t so one interface serves every field regardless of its bit width. The two overloads
/// stay unambiguous because there is no implicit conversion between a name string and an integer.
class EnumMapProvider {
  public:
    virtual ~EnumMapProvider() = default;

    /// @brief Looks up the numeric value for a constant name.
    ///
    /// @param name The value constant name (e.g. "MB_NORMAL")
    /// @return The numeric value if found, or an error describing why the lookup failed
    [[nodiscard]] virtual ChainableResult<std::uint32_t> lookup(const std::string &name) const = 0;

    /// @brief Looks up the constant name for a numeric value.
    ///
    /// @param value The numeric value
    /// @return The constant name if found, or an error describing why the lookup failed
    [[nodiscard]] virtual ChainableResult<std::string> lookup(std::uint32_t value) const = 0;
};

/// @brief Maps schema field names to the provider that names that field's values.
///
/// @details
/// A ProviderMap holds one provider per provider-backed schema field, keyed by the field's name. Field::has_provider()
/// is the single authority for "provider-backed vs raw": a well-formed map contains exactly the schema's
/// has_provider() fields, so consumers may treat has_provider() implying membership as an invariant and panic when it
/// is violated rather than silently falling back to raw handling. The transparent comparator admits lookups by the
/// string_view field-name constants without allocating.
using ProviderMap = std::map<std::string, std::unique_ptr<EnumMapProvider>, std::less<>>;

} // namespace porytiles

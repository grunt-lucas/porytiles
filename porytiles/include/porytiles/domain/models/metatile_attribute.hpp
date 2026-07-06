#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>

#include "porytiles/domain/models/layer.hpp"

namespace porytiles {

namespace attr {

constexpr std::size_t bytes_per_attr_emerald = 2;
constexpr std::size_t bytes_per_attr_firered = 4;

/*
 * Interim field-name constants for the hardcoded emerald/firered attribute layouts. These are the single
 * home for the keys the bin parsers and emitters use while every consumer still forks on base game. They
 * match the names #282's schema inference will produce, so the field map survives untouched once the
 * schema is wired in.
 */
constexpr std::string_view field_behavior = "behavior";
constexpr std::string_view field_terrain = "terrain";
constexpr std::string_view field_attribute_2 = "attribute_2";
constexpr std::string_view field_attribute_3 = "attribute_3";
constexpr std::string_view field_encounter_type = "encounter_type";
constexpr std::string_view field_attribute_5 = "attribute_5";
constexpr std::string_view field_attribute_7 = "attribute_7";

} // namespace attr

/**
 * @brief The attributes of a single metatile, modeled as a map of named field values.
 *
 * @details
 * An attribute is a bag of named field values plus a structural layer type. The bit layout of those
 * fields (their masks, defaults, and how their values are named in a base-game header) is not the
 * attribute's concern; that lives on a Schema. A field absent from the map reads as 0, so an emerald
 * attribute carrying only a behavior and a firered attribute carrying seven fields are the same type,
 * differing only in which keys are populated.
 *
 * layer_type is first-class rather than a schema field because it is structural: it selects which layers
 * a metatile renders and pairs with the layer mode, independent of any base game's attribute encoding.
 * The attribute also does not know whether a field is provider-backed; that too belongs to the Schema.
 */
class MetatileAttribute {
  public:
    MetatileAttribute() = default;

    [[nodiscard]] LayerType layer_type() const
    {
        return layer_type_;
    }

    void layer_type(LayerType layer_type)
    {
        layer_type_ = layer_type;
    }

    /**
     * @brief Returns the value of a named field, or 0 if the field is absent.
     *
     * @param field_name The field name to look up
     * @return The stored value, or 0 when no value has been set for the field
     */
    [[nodiscard]] std::uint32_t field(std::string_view field_name) const
    {
        const auto it = fields_.find(field_name);
        return it != fields_.end() ? it->second : 0;
    }

    /**
     * @brief Sets the value of a named field, inserting or overwriting as needed.
     *
     * @param field_name The field name to set
     * @param value The value to store
     */
    void field(std::string_view field_name, std::uint32_t value)
    {
        fields_.insert_or_assign(std::string{field_name}, value);
    }

    [[nodiscard]] const std::map<std::string, std::uint32_t, std::less<>> &fields() const
    {
        return fields_;
    }

  private:
    LayerType layer_type_{};
    std::map<std::string, std::uint32_t, std::less<>> fields_{};
};

} // namespace porytiles

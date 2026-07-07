#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "porytiles/domain/models/layer.hpp"

namespace porytiles {

namespace attr {

/*
 * Field-name constants for the stock decomp attribute layouts. These are the names the schema inference
 * produces when it scans a stock emerald- or firered-family header, so code and tests that address those
 * fields by name share a single home for the keys.
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
 * fields (their masks, defaults, and how their values are named in a decomp/provider header) is not the
 * attribute's concern; that lives on a Schema. A field absent from the map reads as 0, so an attribute
 * carrying a single field and one carrying seven are the same type, differing only in which keys are
 * populated.
 *
 * layer_type is first-class rather than a schema field because it is structural: it selects which layers
 * a metatile renders and pairs with the layer mode, independent of any particular attribute encoding.
 * The attribute also does not know whether a field is provider-backed; that too belongs to the Schema.
 */
class MetatileAttribute {
  public:
    MetatileAttribute() = default;

    [[nodiscard]] LayerType layer_type() const
    {
        return layer_type_;
    }

    /**
     * @brief Sets the plain (inferred) layer type, clearing any explicit pin.
     *
     * @details
     * This is the inferred-value setter: it records the layer type and drops any prior explicit pin, so a later
     * read through explicit_layer_type() cannot report a stale user pin that layer_type() has since overwritten. A
     * caller that means "the user pinned this" must use explicit_layer_type() instead.
     *
     * @param layer_type The inferred layer type.
     */
    void layer_type(LayerType layer_type)
    {
        layer_type_ = layer_type;
        explicit_layer_type_ = std::nullopt;
    }

    /**
     * @brief Returns the explicit (user-pinned) layer type, if one was set.
     *
     * @details
     * When set, this value pins the layer type against inference: the compile path uses it verbatim instead of the type
     * it would otherwise infer from the metatile's tiles. It is populated from an explicit layer_type cell in the
     * attributes CSV. Producers of inferred layer types (bin parsers, decompiler, metatileizer) must leave it unset so
     * downstream code can tell "the user said so" apart from "we guessed".
     *
     * @return The pinned layer type, or nullopt when the layer type is inferred.
     */
    [[nodiscard]] const std::optional<LayerType> &explicit_layer_type() const
    {
        return explicit_layer_type_;
    }

    /**
     * @brief Pins the layer type to an explicit value.
     *
     * @details
     * Records the pinned value and also updates the plain layer_type so reads through layer_type() stay coherent for
     * code that does not consult the explicit flag.
     *
     * @param layer_type The user-pinned layer type.
     */
    void explicit_layer_type(LayerType layer_type)
    {
        explicit_layer_type_ = layer_type;
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
    std::optional<LayerType> explicit_layer_type_{};
    std::map<std::string, std::uint32_t, std::less<>> fields_{};
};

} // namespace porytiles

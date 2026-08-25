#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "porytiles/domain/models/anim_override_entry.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief What an attributes.csv (if any) said about a role's pin column when this component was loaded.
///
/// @details
/// Recorded per `FieldRole` by the artifact reader when it parses attributes.csv, and consumed by the
/// `merge_prior_layer_type_pin` function. `no_csv` is the default: the decompiler pins every row from the bin.
/// `column_absent` means a CSV was read but the role's active pin column was not in the csv: same effect, the column is
/// added and every row pinned. `column_present` means the active pin column was in the csv: the decompiler preserves
/// each row's prior pin state (a blank cell stays unpinned, a filled cell stays pinned with its value refreshed from
/// the bin).
enum class PriorPinColumnState { no_csv, column_absent, column_present };

/// @brief Converts PriorPinColumnState to its string form.
[[nodiscard]] inline std::string to_string(PriorPinColumnState state)
{
    switch (state) {
    case PriorPinColumnState::no_csv:
        return "no_csv";
    case PriorPinColumnState::column_absent:
        return "column_absent";
    case PriorPinColumnState::column_present:
        return "column_present";
    }
    panic("unhandled PriorPinColumnState value");
}

class PorytilesTilesetComponent {
  public:
    PorytilesTilesetComponent() = default;

    /// @brief Insert a MetatileAttribute to the end of the attribute vector.
    ///
    /// @details
    /// Moves the provided MetatileAttribute into the attribute vector.
    ///
    /// @param metatile_id The id (index) of the metatile to which this attribute belongs
    /// @param attribute The MetatileAttribute to move into the vector.
    /// @pre Attribute at index is not already set
    void insert_attribute(std::size_t metatile_id, MetatileAttribute attribute);

    [[nodiscard]] std::optional<MetatileAttribute> get_attribute(std::size_t metatile_id) const;

    /// @brief Returns the recorded prior pin-column state for a role, or no_csv when none was recorded.
    ///
    /// @param role The role whose prior pin-column state to read
    /// @return The state set by the artifact reader, or PriorPinColumnState::no_csv by default
    [[nodiscard]] PriorPinColumnState prior_pin_column_state(FieldRole role) const;

    /// @brief Records the prior pin-column state for a role (set by the artifact reader after parsing attributes.csv).
    ///
    /// @param role The role the state describes
    /// @param state What the loaded CSV said about the role's active pin column
    void prior_pin_column_state(FieldRole role, PriorPinColumnState state);

    void set_palette(std::size_t palette_index, Palette<Rgba32, palette::max_size> palette);

    [[nodiscard]] const std::optional<Palette<Rgba32, palette::max_size>> &palette_at(std::size_t palette_index) const;

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] ChainableResult<LayerMode> detect_layer_mode(const Rgba32 &extrinsic) const;

    void add_anim(Animation<Rgba32> anim);

    [[nodiscard]] bool has_anim(const std::string &name) const
    {
        return anims_.contains(name);
    }

    [[nodiscard]] const Animation<Rgba32> &anim_for_name(const std::string &name) const
    {
        return anims_.at(name);
    }

    [[nodiscard]] const Image<Rgba32> &bottom() const
    {
        return bottom_;
    }

    void bottom(const Image<Rgba32> &bottom)
    {
        bottom_ = bottom;
    }

    [[nodiscard]] const Image<Rgba32> &middle() const
    {
        return middle_;
    }

    void middle(const Image<Rgba32> &middle)
    {
        middle_ = middle;
    }

    [[nodiscard]] const Image<Rgba32> &top() const
    {
        return top_;
    }

    void top(const Image<Rgba32> &top)
    {
        top_ = top;
    }

    [[nodiscard]] const std::map<std::size_t, MetatileAttribute> &metatile_attributes() const
    {
        return metatile_attributes_;
    }

    [[nodiscard]] const std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> &
    palettes() const
    {
        return palettes_;
    }

    [[nodiscard]] const std::map<std::string, Animation<Rgba32>> &anims() const
    {
        return anims_;
    }

    [[nodiscard]] std::map<std::string, Animation<Rgba32>> &anims()
    {
        return anims_;
    }

    [[nodiscard]] const std::map<std::string, std::vector<AnimOverrideEntry>> &primary_anim_overrides() const
    {
        return primary_anim_overrides_;
    }

    void primary_anim_overrides(std::map<std::string, std::vector<AnimOverrideEntry>> overrides)
    {
        primary_anim_overrides_ = std::move(overrides);
    }

  private:
    Image<Rgba32> bottom_;
    Image<Rgba32> middle_;
    Image<Rgba32> top_;
    std::map<std::size_t, MetatileAttribute> metatile_attributes_;
    std::map<FieldRole, PriorPinColumnState> prior_pin_column_states_;
    std::array<std::optional<Palette<Rgba32, palette::max_size>>, palette::num_palettes> palettes_;
    std::map<std::string, Animation<Rgba32>> anims_;
    std::map<std::string, std::vector<AnimOverrideEntry>> primary_anim_overrides_;
};

} // namespace porytiles

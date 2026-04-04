#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "porytiles2/domain/models/anim_override_entry.hpp"
#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  public:
    PorytilesTilesetComponent() = default;

    /**
     * @brief Insert a MetatileAttribute to the end of the attribute vector.
     *
     * @details
     * Moves the provided MetatileAttribute into the attribute vector.
     *
     * @param metatile_id The id (index) of the metatile to which this attribute belongs
     * @param attribute The MetatileAttribute to move into the vector.
     * @pre Attribute at index is not already set
     */
    void insert_attribute(std::size_t metatile_id, MetatileAttribute attribute);

    [[nodiscard]] std::optional<MetatileAttribute> get_attribute(std::size_t metatile_id) const;

    void set_pal(std::size_t pal_index, Palette<Rgba32, pal::max_size> pal);

    [[nodiscard]] const std::optional<Palette<Rgba32, pal::max_size>> &pal_at(std::size_t pal_index) const;

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

    [[nodiscard]] const std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> &pals() const
    {
        return pals_;
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
    std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> pals_;
    std::map<std::string, Animation<Rgba32>> anims_;
    std::map<std::string, std::vector<AnimOverrideEntry>> primary_anim_overrides_;
};

} // namespace porytiles2

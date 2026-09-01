#include "porytiles/domain/services/tileset_creator.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

#include "gtest/gtest.h"

#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "support/mock_domain_config.hpp"

using namespace porytiles;

namespace {

class StubBehaviorProvider final : public EnumMapProvider {
  public:
    [[nodiscard]] ChainableResult<std::uint32_t> lookup(const std::string &name) const override
    {
        auto it = name_to_value_.find(name);
        if (it == name_to_value_.end()) {
            return FormattableError{"unknown behavior: {}", FormatParam{name}};
        }
        return it->second;
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint32_t value) const override
    {
        for (const auto &[name, val] : name_to_value_) {
            if (val == value) {
                return name;
            }
        }
        return FormattableError{"unknown behavior value: {}", FormatParam{value}};
    }

  private:
    std::unordered_map<std::string, std::uint32_t> name_to_value_{{"MB_NORMAL", 0x00}, {"MB_TALL_GRASS", 0x02}};
};

bool metatile_region_is_solid(const Image<Rgba32> &img, std::size_t metatile_index, const Rgba32 &color)
{
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            if (img.at(row, col_offset + col) != color) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

TEST(TilesetCreatorTests, PrimarySampleWithAnims)
{
    MockDomainConfig config{};
    StubBehaviorProvider behaviors{};
    TilesetCreator creator{&config, &behaviors};

    auto result = creator.create_sample_primary_porytiles_component("gTileset_Test");

    ASSERT_TRUE(result.has_value());
    const auto &component = result.value();
    EXPECT_TRUE(component->anims().contains("flower"));
    EXPECT_FALSE(metatile_region_is_solid(component->middle(), 2, rgba_magenta));
}

TEST(TilesetCreatorTests, PrimarySampleWithoutAnims)
{
    MockDomainConfig config{};
    config.create_sample_anims = false;
    StubBehaviorProvider behaviors{};
    TilesetCreator creator{&config, &behaviors};

    auto result = creator.create_sample_primary_porytiles_component("gTileset_Test");

    ASSERT_TRUE(result.has_value());
    const auto &component = result.value();
    EXPECT_TRUE(component->anims().empty());
    EXPECT_TRUE(metatile_region_is_solid(component->middle(), 2, rgba_magenta));
    EXPECT_FALSE(metatile_region_is_solid(component->bottom(), 2, rgba_magenta));
}

TEST(TilesetCreatorTests, SecondarySampleWithAnims)
{
    MockDomainConfig config{};
    StubBehaviorProvider behaviors{};
    TilesetCreator creator{&config, &behaviors};

    auto result = creator.create_sample_secondary_porytiles_component("gTileset_Test");

    ASSERT_TRUE(result.has_value());
    const auto &component = result.value();
    EXPECT_TRUE(component->anims().empty());
    EXPECT_FALSE(metatile_region_is_solid(component->middle(), 2, rgba_magenta));
}

TEST(TilesetCreatorTests, SecondarySampleWithoutAnims)
{
    MockDomainConfig config{};
    config.create_sample_anims = false;
    StubBehaviorProvider behaviors{};
    TilesetCreator creator{&config, &behaviors};

    auto result = creator.create_sample_secondary_porytiles_component("gTileset_Test");

    ASSERT_TRUE(result.has_value());
    const auto &component = result.value();
    EXPECT_TRUE(component->anims().empty());
    EXPECT_TRUE(metatile_region_is_solid(component->middle(), 2, rgba_magenta));
    EXPECT_FALSE(metatile_region_is_solid(component->bottom(), 2, rgba_magenta));
}

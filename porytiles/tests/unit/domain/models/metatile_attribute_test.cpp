#include "gtest/gtest.h"

#include <ranges>
#include <string>
#include <vector>

#include "porytiles/domain/models/metatile_attribute.hpp"

using namespace porytiles;

TEST(MetatileAttributeTest, DefaultConstructedHasNormalLayerTypeAndNoFields)
{
    MetatileAttribute attribute{};
    EXPECT_EQ(attribute.layer_type(), LayerType::normal);
    EXPECT_TRUE(attribute.fields().empty());
}

TEST(MetatileAttributeTest, UnsetFieldReadsZero)
{
    MetatileAttribute attribute{};
    EXPECT_EQ(attribute.field(attr::field_behavior), 0u);
    EXPECT_EQ(attribute.field(attr::field_terrain), 0u);
    EXPECT_EQ(attribute.field(attr::field_attribute_7), 0u);
}

TEST(MetatileAttributeTest, SetFieldInsertsThenOverwrites)
{
    MetatileAttribute attribute{};

    attribute.field(attr::field_behavior, 42);
    EXPECT_EQ(attribute.field(attr::field_behavior), 42u);
    EXPECT_EQ(attribute.fields().size(), 1u);

    attribute.field(attr::field_behavior, 511);
    EXPECT_EQ(attribute.field(attr::field_behavior), 511u);
    EXPECT_EQ(attribute.fields().size(), 1u);
}

TEST(MetatileAttributeTest, LayerTypeRoundTrips)
{
    MetatileAttribute attribute{};

    attribute.layer_type(LayerType::covered);
    EXPECT_EQ(attribute.layer_type(), LayerType::covered);

    attribute.layer_type(LayerType::split);
    EXPECT_EQ(attribute.layer_type(), LayerType::split);
}

TEST(MetatileAttributeTest, FieldsIterateInDeterministicOrder)
{
    MetatileAttribute attribute{};
    attribute.field(attr::field_terrain, 1);
    attribute.field(attr::field_behavior, 2);
    attribute.field(attr::field_encounter_type, 3);

    // std::map with std::less<> keeps keys sorted, so iteration order is stable regardless of insertion order.
    std::vector<std::string> names;
    for (const auto &name : attribute.fields() | std::views::keys) {
        names.push_back(name);
    }

    const std::vector<std::string> expected{
        std::string{attr::field_behavior}, std::string{attr::field_encounter_type}, std::string{attr::field_terrain}};
    EXPECT_EQ(names, expected);
}

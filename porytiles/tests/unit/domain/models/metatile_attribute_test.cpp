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
    EXPECT_EQ(attribute.field(attribute::field_behavior), 0u);
    EXPECT_EQ(attribute.field(attribute::field_terrain), 0u);
    EXPECT_EQ(attribute.field(attribute::field_attribute_7), 0u);
}

TEST(MetatileAttributeTest, SetFieldInsertsThenOverwrites)
{
    MetatileAttribute attribute{};

    attribute.field(attribute::field_behavior, 42);
    EXPECT_EQ(attribute.field(attribute::field_behavior), 42u);
    EXPECT_EQ(attribute.fields().size(), 1u);

    attribute.field(attribute::field_behavior, 511);
    EXPECT_EQ(attribute.field(attribute::field_behavior), 511u);
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

TEST(MetatileAttributeTest, ExplicitLayerTypeUnsetByDefault)
{
    MetatileAttribute attribute{};
    EXPECT_FALSE(attribute.explicit_layer_type().has_value());
}

TEST(MetatileAttributeTest, ExplicitLayerTypeSetterAlsoUpdatesLayerType)
{
    MetatileAttribute attribute{};

    attribute.explicit_layer_type(LayerType::split);
    ASSERT_TRUE(attribute.explicit_layer_type().has_value());
    EXPECT_EQ(attribute.explicit_layer_type().value(), LayerType::split);
    // The plain layer_type stays coherent for code that does not consult the explicit flag.
    EXPECT_EQ(attribute.layer_type(), LayerType::split);
}

TEST(MetatileAttributeTest, PlainLayerTypeSetterLeavesExplicitUnset)
{
    MetatileAttribute attribute{};

    attribute.layer_type(LayerType::covered);
    EXPECT_EQ(attribute.layer_type(), LayerType::covered);
    // A producer of an inferred value must not accidentally pin the layer type.
    EXPECT_FALSE(attribute.explicit_layer_type().has_value());
}

TEST(MetatileAttributeTest, ExplicitLayerTypeSurvivesCopy)
{
    MetatileAttribute attribute{};
    attribute.explicit_layer_type(LayerType::covered);

    MetatileAttribute copy = attribute;
    ASSERT_TRUE(copy.explicit_layer_type().has_value());
    EXPECT_EQ(copy.explicit_layer_type().value(), LayerType::covered);
    EXPECT_EQ(copy.layer_type(), LayerType::covered);
}

TEST(MetatileAttributeTest, FieldsIterateInDeterministicOrder)
{
    MetatileAttribute attribute{};
    attribute.field(attribute::field_terrain, 1);
    attribute.field(attribute::field_behavior, 2);
    attribute.field(attribute::field_encounter_type, 3);

    // std::map with std::less<> keeps keys sorted, so iteration order is stable regardless of insertion order.
    std::vector<std::string> names;
    for (const auto &name : attribute.fields() | std::views::keys) {
        names.push_back(name);
    }

    const std::vector<std::string> expected{
        std::string{attribute::field_behavior},
        std::string{attribute::field_encounter_type},
        std::string{attribute::field_terrain}};
    EXPECT_EQ(names, expected);
}

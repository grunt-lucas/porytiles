#include "gtest/gtest.h"

#include "porytiles2/domain/models/metatile_attribute.hpp"

using namespace porytiles2;

TEST(MetatileAttributeTest, DefaultConstructorZerosAllFields)
{
    MetatileAttribute attr{};
    EXPECT_EQ(attr.behavior(), 0);
    EXPECT_EQ(attr.terrain(), 0);
    EXPECT_EQ(attr.encounter_type(), 0);
    EXPECT_EQ(attr.attribute_2(), 0);
    EXPECT_EQ(attr.attribute_3(), 0);
    EXPECT_EQ(attr.attribute_5(), 0);
    EXPECT_FALSE(attr.attribute_7());
}

TEST(MetatileAttributeTest, EmeraldConstructorLeavesFireRedFieldsZero)
{
    MetatileAttribute attr{LayerType::covered, 42};
    EXPECT_EQ(attr.layer_type(), LayerType::covered);
    EXPECT_EQ(attr.behavior(), 42);
    EXPECT_EQ(attr.terrain(), 0);
    EXPECT_EQ(attr.encounter_type(), 0);
    EXPECT_EQ(attr.attribute_2(), 0);
    EXPECT_EQ(attr.attribute_3(), 0);
    EXPECT_EQ(attr.attribute_5(), 0);
    EXPECT_FALSE(attr.attribute_7());
}

TEST(MetatileAttributeTest, FullConstructorPopulatesAllFields)
{
    MetatileAttribute attr{LayerType::split, 255, 17, 5, 9, 33, 2, true};
    EXPECT_EQ(attr.layer_type(), LayerType::split);
    EXPECT_EQ(attr.behavior(), 255);
    EXPECT_EQ(attr.terrain(), 17);
    EXPECT_EQ(attr.encounter_type(), 5);
    EXPECT_EQ(attr.attribute_2(), 9);
    EXPECT_EQ(attr.attribute_3(), 33);
    EXPECT_EQ(attr.attribute_5(), 2);
    EXPECT_TRUE(attr.attribute_7());
}

TEST(MetatileAttributeTest, AccessorMutatorRoundTrips)
{
    MetatileAttribute attr{};

    attr.layer_type(LayerType::covered);
    EXPECT_EQ(attr.layer_type(), LayerType::covered);

    attr.behavior(511);
    EXPECT_EQ(attr.behavior(), 511);

    attr.terrain(31);
    EXPECT_EQ(attr.terrain(), 31);

    attr.encounter_type(7);
    EXPECT_EQ(attr.encounter_type(), 7);

    attr.attribute_2(15);
    EXPECT_EQ(attr.attribute_2(), 15);

    attr.attribute_3(63);
    EXPECT_EQ(attr.attribute_3(), 63);

    attr.attribute_5(3);
    EXPECT_EQ(attr.attribute_5(), 3);

    attr.attribute_7(true);
    EXPECT_TRUE(attr.attribute_7());
    attr.attribute_7(false);
    EXPECT_FALSE(attr.attribute_7());
}

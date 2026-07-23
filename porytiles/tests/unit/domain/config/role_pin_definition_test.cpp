#include "porytiles/domain/config/role_pin_definition.hpp"

#include <optional>

#include "gtest/gtest.h"

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

Schema emerald_schema()
{
    return std::move(
               Schema::create(
                   {Field{"behavior", 0x00FF}, Field{"layer_type", 0xF000, 0, std::nullopt, FieldRole::layer_type}}, 2))
        .value();
}

TEST(ValidateRolePinsAgainstSchemaTest, PassesWhenNoColumnCollision)
{
    const Schema schema = emerald_schema();
    const PlainTextFormatter formatter;
    const RolePinDefinitions pins{{FieldRole::layer_type, std::nullopt}};

    EXPECT_TRUE(validate_role_pins_against_schema(pins, schema, formatter).has_value());
}

// A pin for a role no schema field carries is allowed silently: the pin still steers dual-layerize even when the
// packed attribute has no layer_type field to write.
TEST(ValidateRolePinsAgainstSchemaTest, PassesWhenSchemaDoesNotCarryRole)
{
    const Schema schema = std::move(Schema::create({Field{"behavior", 0x00FF}}, 2)).value();
    const PlainTextFormatter formatter;
    const RolePinDefinitions pins{{FieldRole::layer_type, std::nullopt}};

    EXPECT_TRUE(validate_role_pins_against_schema(pins, schema, formatter).has_value());
}

TEST(ValidateRolePinsAgainstSchemaTest, PassesWhenSchemaIsEmpty)
{
    const Schema schema = std::move(Schema::create({}, 2)).value();
    const PlainTextFormatter formatter;
    const RolePinDefinitions pins{{FieldRole::layer_type, std::nullopt}};

    EXPECT_TRUE(validate_role_pins_against_schema(pins, schema, formatter).has_value());
}

TEST(ValidateRolePinsAgainstSchemaTest, ErrorsWhenColumnCollidesWithValueField)
{
    const Schema schema = emerald_schema();
    const PlainTextFormatter formatter;
    const RolePinDefinitions pins{{FieldRole::layer_type, "behavior"}};

    const auto result = validate_role_pins_against_schema(pins, schema, formatter);
    ASSERT_FALSE(result.has_value());
    const std::string error = result.error().join(formatter);
    EXPECT_NE(error.find("collides"), std::string::npos) << error;
    EXPECT_NE(error.find("behavior"), std::string::npos) << error;
}

} // namespace
} // namespace porytiles

#include "gtest/gtest.h"

#include <string>

#include "porytiles/domain/models/layer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

TEST(LayerTypeCsvTokenTest, TokensAreLowercaseAndStable)
{
    EXPECT_EQ(layer_type_csv_token(LayerType::normal), "normal");
    EXPECT_EQ(layer_type_csv_token(LayerType::covered), "covered");
    EXPECT_EQ(layer_type_csv_token(LayerType::split), "split");
}

TEST(LayerTypeCsvTokenTest, RoundTripsThroughParse)
{
    for (const auto layer_type : {LayerType::normal, LayerType::covered, LayerType::split}) {
        const auto parsed = layer_type_from_csv_token(layer_type_csv_token(layer_type));
        ASSERT_TRUE(parsed.has_value());
        EXPECT_EQ(parsed.value(), layer_type);
    }
}

TEST(LayerTypeCsvTokenTest, ParseIsCaseInsensitive)
{
    EXPECT_EQ(layer_type_from_csv_token("Normal").value(), LayerType::normal);
    EXPECT_EQ(layer_type_from_csv_token("COVERED").value(), LayerType::covered);
    EXPECT_EQ(layer_type_from_csv_token("SpLiT").value(), LayerType::split);
}

TEST(LayerTypeCsvTokenTest, InvalidTokenErrorsAndListsValidTokens)
{
    PlainTextFormatter formatter{};
    const auto result = layer_type_from_csv_token("sideways");
    ASSERT_FALSE(result.has_value());

    std::string text;
    for (const auto &err : result.chain()) {
        for (const auto &line : err->details(formatter)) {
            text += line + "\n";
        }
    }
    EXPECT_NE(text.find("normal"), std::string::npos);
    EXPECT_NE(text.find("covered"), std::string::npos);
    EXPECT_NE(text.find("split"), std::string::npos);
}

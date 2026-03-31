#include "gtest/gtest.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "porytiles2/domain/models/anim_override_entry.hpp"
#include "porytiles2/domain/models/anim_params.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/infra/services/anim_json_parser.hpp"
#include "porytiles2/utilities/dynamic_cased_name.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

using namespace porytiles2;

namespace {

const std::filesystem::path kTestDir = std::filesystem::temp_directory_path() / "porytiles2_test_anim_json_parser";

class AnimJsonParserOverrideTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        std::filesystem::create_directories(kTestDir);
        formatter_ = std::make_unique<PlainTextFormatter>();
        parser_ = std::make_unique<AnimJsonParser>(formatter_.get());
    }

    void TearDown() override
    {
        std::filesystem::remove_all(kTestDir);
    }

    void write_json_file(const std::filesystem::path &path, const std::string &content)
    {
        std::ofstream out{path};
        out << content;
    }

    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<AnimJsonParser> parser_;
};

TEST_F(AnimJsonParserOverrideTest, ParseOverridesFromJson)
{
    const auto json_path = kTestDir / "anim.json";
    write_json_file(json_path, R"({
  "water": {
    "frame_factor": 16,
    "frames": ["0", "1"],
    "overrides": [
      {
        "id": 5,
        "layer": "bottom",
        "subtile": "northwest",
        "frame_subtile": 0,
        "pal_index": 2,
        "hflip": false,
        "vflip": false
      },
      {
        "id": 10,
        "layer": "top",
        "subtile": "southeast",
        "frame_subtile": 3,
        "pal_index": 4,
        "hflip": true,
        "vflip": true
      }
    ]
  }
})");

    auto result = parser_->parse(json_path);
    ASSERT_TRUE(result.has_value());

    const auto &params_map = result.value();
    ASSERT_EQ(params_map.size(), 1);

    const auto &water_params = params_map.at(DynamicCasedName{"water"});
    const auto &overrides = water_params.overrides();
    ASSERT_EQ(overrides.size(), 2);

    // First override
    EXPECT_EQ(overrides[0].metatile_id, 5);
    EXPECT_EQ(overrides[0].layer, metatile::Layer::bottom);
    EXPECT_EQ(overrides[0].subtile, metatile::Subtile::northwest);
    EXPECT_EQ(overrides[0].frame_subtile, 0);
    EXPECT_EQ(overrides[0].pal_index, 2);
    EXPECT_FALSE(overrides[0].h_flip);
    EXPECT_FALSE(overrides[0].v_flip);

    // Second override
    EXPECT_EQ(overrides[1].metatile_id, 10);
    EXPECT_EQ(overrides[1].layer, metatile::Layer::top);
    EXPECT_EQ(overrides[1].subtile, metatile::Subtile::southeast);
    EXPECT_EQ(overrides[1].frame_subtile, 3);
    EXPECT_EQ(overrides[1].pal_index, 4);
    EXPECT_TRUE(overrides[1].h_flip);
    EXPECT_TRUE(overrides[1].v_flip);
}

TEST_F(AnimJsonParserOverrideTest, ParseEmptyOverrides)
{
    const auto json_path = kTestDir / "anim.json";
    write_json_file(json_path, R"({
  "flower": {
    "frames": ["0", "1"],
    "overrides": []
  }
})");

    auto result = parser_->parse(json_path);
    ASSERT_TRUE(result.has_value());

    const auto &params_map = result.value();
    const auto &flower_params = params_map.at(DynamicCasedName{"flower"});
    EXPECT_TRUE(flower_params.overrides().empty());
}

TEST_F(AnimJsonParserOverrideTest, ParseAnimationWithoutOverrides)
{
    const auto json_path = kTestDir / "anim.json";
    write_json_file(json_path, R"({
  "flower": {
    "frame_factor": 8,
    "frames": ["0", "1", "2"]
  }
})");

    auto result = parser_->parse(json_path);
    ASSERT_TRUE(result.has_value());

    const auto &params_map = result.value();
    const auto &flower_params = params_map.at(DynamicCasedName{"flower"});
    EXPECT_TRUE(flower_params.overrides().empty());
}

TEST_F(AnimJsonParserOverrideTest, ParseAllLayerValues)
{
    const auto json_path = kTestDir / "anim.json";
    write_json_file(json_path, R"({
  "water": {
    "frames": ["0"],
    "overrides": [
      { "id": 0, "layer": "bottom", "subtile": "northwest", "frame_subtile": 0, "pal_index": 0, "hflip": false, "vflip": false },
      { "id": 1, "layer": "middle", "subtile": "northwest", "frame_subtile": 0, "pal_index": 0, "hflip": false, "vflip": false },
      { "id": 2, "layer": "top", "subtile": "northwest", "frame_subtile": 0, "pal_index": 0, "hflip": false, "vflip": false }
    ]
  }
})");

    auto result = parser_->parse(json_path);
    ASSERT_TRUE(result.has_value());

    const auto &overrides = result.value().at(DynamicCasedName{"water"}).overrides();
    ASSERT_EQ(overrides.size(), 3);
    EXPECT_EQ(overrides[0].layer, metatile::Layer::bottom);
    EXPECT_EQ(overrides[1].layer, metatile::Layer::middle);
    EXPECT_EQ(overrides[2].layer, metatile::Layer::top);
}

TEST_F(AnimJsonParserOverrideTest, ParseAllSubtileValues)
{
    const auto json_path = kTestDir / "anim.json";
    write_json_file(json_path, R"({
  "water": {
    "frames": ["0"],
    "overrides": [
      { "id": 0, "layer": "bottom", "subtile": "northwest", "frame_subtile": 0, "pal_index": 0, "hflip": false, "vflip": false },
      { "id": 0, "layer": "bottom", "subtile": "northeast", "frame_subtile": 0, "pal_index": 0, "hflip": false, "vflip": false },
      { "id": 0, "layer": "bottom", "subtile": "southwest", "frame_subtile": 0, "pal_index": 0, "hflip": false, "vflip": false },
      { "id": 0, "layer": "bottom", "subtile": "southeast", "frame_subtile": 0, "pal_index": 0, "hflip": false, "vflip": false }
    ]
  }
})");

    auto result = parser_->parse(json_path);
    ASSERT_TRUE(result.has_value());

    const auto &overrides = result.value().at(DynamicCasedName{"water"}).overrides();
    ASSERT_EQ(overrides.size(), 4);
    EXPECT_EQ(overrides[0].subtile, metatile::Subtile::northwest);
    EXPECT_EQ(overrides[1].subtile, metatile::Subtile::northeast);
    EXPECT_EQ(overrides[2].subtile, metatile::Subtile::southwest);
    EXPECT_EQ(overrides[3].subtile, metatile::Subtile::southeast);
}

TEST_F(AnimJsonParserOverrideTest, RoundTripOverrides)
{
    // Build params with overrides
    AnimParams params;
    params.cased_name(DynamicCasedName{"water"});
    params.frame_factor(16);
    params.frame_names({DynamicCasedName{"0"}, DynamicCasedName{"1"}});
    params.frame_order({DynamicCasedName{"0"}, DynamicCasedName{"1"}});

    std::vector<AnimOverrideEntry> overrides;
    overrides.push_back(
        AnimOverrideEntry{
            .metatile_id = 5,
            .layer = metatile::Layer::bottom,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 2,
            .h_flip = false,
            .v_flip = false});
    overrides.push_back(
        AnimOverrideEntry{
            .metatile_id = 10,
            .layer = metatile::Layer::top,
            .subtile = metatile::Subtile::southeast,
            .frame_subtile = 3,
            .pal_index = 4,
            .h_flip = true,
            .v_flip = true});
    params.overrides(std::move(overrides));

    // Write to file
    const auto json_path = kTestDir / "round_trip.json";
    std::map<DynamicCasedName, AnimParams> params_map;
    params_map.insert({DynamicCasedName{"water"}, std::move(params)});

    auto write_result = parser_->write(json_path, params_map);
    ASSERT_TRUE(write_result.has_value());

    // Read back
    auto read_result = parser_->parse(json_path);
    ASSERT_TRUE(read_result.has_value());

    const auto &read_params = read_result.value().at(DynamicCasedName{"water"});
    const auto &read_overrides = read_params.overrides();
    ASSERT_EQ(read_overrides.size(), 2);

    // Verify first override survived round-trip
    EXPECT_EQ(read_overrides[0].metatile_id, 5);
    EXPECT_EQ(read_overrides[0].layer, metatile::Layer::bottom);
    EXPECT_EQ(read_overrides[0].subtile, metatile::Subtile::northwest);
    EXPECT_EQ(read_overrides[0].frame_subtile, 0);
    EXPECT_EQ(read_overrides[0].pal_index, 2);
    EXPECT_FALSE(read_overrides[0].h_flip);
    EXPECT_FALSE(read_overrides[0].v_flip);

    // Verify second override survived round-trip
    EXPECT_EQ(read_overrides[1].metatile_id, 10);
    EXPECT_EQ(read_overrides[1].layer, metatile::Layer::top);
    EXPECT_EQ(read_overrides[1].subtile, metatile::Subtile::southeast);
    EXPECT_EQ(read_overrides[1].frame_subtile, 3);
    EXPECT_EQ(read_overrides[1].pal_index, 4);
    EXPECT_TRUE(read_overrides[1].h_flip);
    EXPECT_TRUE(read_overrides[1].v_flip);
}

TEST_F(AnimJsonParserOverrideTest, NoOverridesWhenEmpty)
{
    AnimParams params;
    params.cased_name(DynamicCasedName{"flower"});
    params.frame_names({DynamicCasedName{"0"}, DynamicCasedName{"1"}});
    params.frame_order({DynamicCasedName{"0"}, DynamicCasedName{"1"}});

    const auto json_path = kTestDir / "no_overrides.json";
    std::map<DynamicCasedName, AnimParams> params_map;
    params_map.insert({DynamicCasedName{"flower"}, std::move(params)});

    auto write_result = parser_->write(json_path, params_map);
    ASSERT_TRUE(write_result.has_value());

    // Read the raw JSON to verify "overrides" key is absent
    std::ifstream in{json_path};
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("overrides"), std::string::npos);
}

TEST_F(AnimJsonParserOverrideTest, OverrideFieldsInCanonicalOrder)
{
    AnimParams params;
    params.cased_name(DynamicCasedName{"water"});
    params.frame_names({DynamicCasedName{"0"}});
    params.frame_order({DynamicCasedName{"0"}});

    std::vector<AnimOverrideEntry> overrides;
    overrides.push_back(
        AnimOverrideEntry{
            .metatile_id = 1,
            .layer = metatile::Layer::bottom,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 0,
            .h_flip = false,
            .v_flip = false});
    params.overrides(std::move(overrides));

    const auto json_path = kTestDir / "field_order.json";
    std::map<DynamicCasedName, AnimParams> params_map;
    params_map.insert({DynamicCasedName{"water"}, std::move(params)});

    auto write_result = parser_->write(json_path, params_map);
    ASSERT_TRUE(write_result.has_value());

    // Read the raw JSON and verify field order is: id, layer, subtile, frame_subtile, pal_index, hflip, vflip
    std::ifstream in{json_path};
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    auto pos_id = content.find("\"id\"");
    auto pos_layer = content.find("\"layer\"");
    auto pos_subtile = content.find("\"subtile\"");
    auto pos_frame_subtile = content.find("\"frame_subtile\"");
    auto pos_pal_index = content.find("\"pal_index\"");
    auto pos_hflip = content.find("\"hflip\"");
    auto pos_vflip = content.find("\"vflip\"");

    ASSERT_NE(pos_id, std::string::npos);
    EXPECT_LT(pos_id, pos_layer);
    EXPECT_LT(pos_layer, pos_subtile);
    EXPECT_LT(pos_subtile, pos_frame_subtile);
    EXPECT_LT(pos_frame_subtile, pos_pal_index);
    EXPECT_LT(pos_pal_index, pos_hflip);
    EXPECT_LT(pos_hflip, pos_vflip);
}

} // namespace

#include "gtest/gtest.h"

#include <tuple>
#include <vector>

#include "porytiles2/templates/AnyMap.hpp"

using namespace porytiles;

TEST(AnyMapTests, PutTryGetShouldWork) {
  AnyMap map{};

  map.put("key1", 22);
  map.put("key2", std::string{"foobar"});
  map.put("key3", std::vector{1, 2, 3});

  EXPECT_EQ(map.try_get<int>("key1"), 22);

  EXPECT_EQ(map.try_get<std::string>("key2"), "foobar");

  EXPECT_FALSE(map.try_get<int>("key3").has_value());

  const auto expected = std::vector{1, 2, 3};
  EXPECT_EQ(map.get<std::vector<int>>("key3"), expected);

  EXPECT_FALSE(map.try_get<double>("key4").has_value());

  EXPECT_EXIT(std::ignore = map.get<int>("key25"), ::testing::KilledBySignal(SIGABRT),
              "Key not found: key25");
}

TEST(AnyMapTests, GetShouldPanicOnWrongType) {
  AnyMap map{};

  map.put("key1", 22);
  map.put("key2", std::string{"foobar"});
  map.put("key3", std::vector{1, 2, 3});

  EXPECT_EXIT(std::ignore = map.get<int>("key2"), ::testing::KilledBySignal(SIGABRT),
              "Invalid type requested for key: key2");
}

#include "gtest/gtest.h"

#include <tuple>
#include <vector>

#include "porytiles2/templates/AnyMap.hpp"

using namespace porytiles;

TEST(AnyMapTests, PutTryGetShouldWork) {
  AnyMap map{};

  map.Put("key1", 22);
  map.Put("key2", std::string{"foobar"});
  map.Put("key3", std::vector{1, 2, 3});

  EXPECT_EQ(map.Try<int>("key1"), 22);

  EXPECT_EQ(map.Try<std::string>("key2"), "foobar");

  EXPECT_FALSE(map.Try<int>("key3").has_value());

  const auto expected = std::vector{1, 2, 3};
  EXPECT_EQ(map.Get<std::vector<int>>("key3"), expected);

  EXPECT_FALSE(map.Try<double>("key4").has_value());

  EXPECT_EXIT(std::ignore = map.Get<int>("key25"),
              ::testing::KilledBySignal(SIGABRT), "Key not found: key25");
}

TEST(AnyMapTests, GetShouldPanicOnWrongType) {
  AnyMap map{};

  map.Put("key1", 22);
  map.Put("key2", std::string{"foobar"});
  map.Put("key3", std::vector{1, 2, 3});

  EXPECT_EXIT(std::ignore = map.Get<int>("key2"),
              ::testing::KilledBySignal(SIGABRT),
              "Invalid type requested for key: key2");
}

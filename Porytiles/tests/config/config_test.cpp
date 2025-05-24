#include <gtest/gtest.h>

#include <tuple>
#include <vector>

#include <porytiles/config/config.hpp>

using namespace porytiles;

TEST(ConfigTests, ConfigPutTryGetShouldWork) {
    Config config{};

    config.Put("key1", 22);
    config.Put("key2", std::string{"foobar"});
    config.Put("key3", std::vector{1, 2, 3});

    ASSERT_EQ(config.Try<int>("key1"), 22);
    ASSERT_EQ(config.Try<std::string>("key2"), "foobar");
    const auto expected = std::vector{1, 2, 3};
    ASSERT_EQ(config.Get<std::vector<int>>("key3"), expected);
    ASSERT_FALSE(config.Try<double>("key4").has_value());
    ASSERT_EXIT(std::ignore = config.Get<int>("key25"), ::testing::KilledBySignal(SIGABRT), "Key not found: key25");
}

TEST(ConfigTests, ConfigGetShouldPanicOnWrongType) {
    Config config{};

    config.Put("key1", 22);
    config.Put("key2", std::string{"foobar"});
    config.Put("key3", std::vector{1, 2, 3});

    ASSERT_EXIT(std::ignore = config.Get<int>("key2"), ::testing::KilledBySignal(SIGABRT),
                "Invalid type requested for key: key2");
}

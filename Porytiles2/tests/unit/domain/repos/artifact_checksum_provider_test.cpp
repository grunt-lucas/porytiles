#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <chrono>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/repos/artifact_key.hpp"
#include "porytiles2/domain/services/artifact_checksum_provider.hpp"

using namespace porytiles2;
using namespace ::testing;

class MockArtifactChecksumProvider : public ArtifactChecksumProvider {
  public:
    MOCK_METHOD(
        (std::unordered_map<ArtifactKey, std::string>),
        compute_artifact_checksums,
        (const std::string &tileset_name),
        (const, override));
    MOCK_METHOD(
        (std::unordered_map<ArtifactKey, std::string>),
        load_cached_checksums,
        (const std::string &tileset_name),
        (const, override));
    MOCK_METHOD(
        (Result<void>),
        cache_checksums,
        ((const std::string &tileset_name), (const std::unordered_map<ArtifactKey, std::string> &checksums)),
        (const, override));
};

class ArtifactChecksumProviderTest : public ::testing::Test {
  protected:
    MockArtifactChecksumProvider provider_;
    std::string test_tileset_name_ = "test_tileset";
};

// Tests for find_unsynced_artifacts method

TEST_F(ArtifactChecksumProviderTest, FindUnsyncedArtifacts_AllChecksumsMatch_ReturnsEmpty)
{
    // Setup: All checksums match between current and cached
    std::vector<ArtifactKey> artifact_keys = {ArtifactKey{"key1"}, ArtifactKey{"key2"}, ArtifactKey{"key3"}};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}, {ArtifactKey{"key3"}, "checksum3"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}, {ArtifactKey{"key3"}, "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_tileset_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result.empty());
}

TEST_F(ArtifactChecksumProviderTest, FindUnsyncedArtifacts_SomeChecksumsDoNotMatch_ReturnsUnsyncedKeys)
{
    // Setup: Some checksums don't match between current and cached
    std::vector<ArtifactKey> artifact_keys = {ArtifactKey{"key1"}, ArtifactKey{"key2"}, ArtifactKey{"key3"}};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {
        {ArtifactKey{"key1"}, "checksum1"},
        {ArtifactKey{"key2"}, "checksum2_modified"},
        {ArtifactKey{"key3"}, "checksum3"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}, {ArtifactKey{"key3"}, "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_tileset_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], ArtifactKey{"key2"});
}

TEST_F(ArtifactChecksumProviderTest, FindUnsyncedArtifacts_AllChecksumsDoNotMatch_ReturnsAllKeys)
{
    // Setup: All checksums don't match between current and cached
    std::vector<ArtifactKey> artifact_keys = {ArtifactKey{"key1"}, ArtifactKey{"key2"}, ArtifactKey{"key3"}};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {
        {ArtifactKey{"key1"}, "checksum1_modified"},
        {ArtifactKey{"key2"}, "checksum2_modified"},
        {ArtifactKey{"key3"}, "checksum3_modified"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}, {ArtifactKey{"key3"}, "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_tileset_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 3);
    EXPECT_THAT(result, UnorderedElementsAre(ArtifactKey{"key1"}, ArtifactKey{"key2"}, ArtifactKey{"key3"}));
}

TEST_F(ArtifactChecksumProviderTest, FindUnsyncedArtifacts_MissingCurrentChecksum_ReturnsUnsyncedKey)
{
    // Setup: Key is missing from current checksums but present in cached
    std::vector<ArtifactKey> artifact_keys = {ArtifactKey{"key1"}, ArtifactKey{"key2"}};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}}; // key2 missing
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_tileset_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], ArtifactKey{"key2"});
}

TEST_F(ArtifactChecksumProviderTest, FindUnsyncedArtifacts_MissingCachedChecksum_ReturnsUnsyncedKey)
{
    // Setup: Key is missing from cached checksums but present in current
    std::vector<ArtifactKey> artifact_keys = {ArtifactKey{"key1"}, ArtifactKey{"key2"}};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}}; // key2 missing

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_tileset_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], ArtifactKey{"key2"});
}

TEST_F(ArtifactChecksumProviderTest, FindUnsyncedArtifacts_EmptyArtifactKeysList_ReturnsEmpty)
{
    // Setup: Empty artifact keys list
    std::vector<ArtifactKey> artifact_keys = {};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {{ArtifactKey{"key1"}, "checksum1"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {{ArtifactKey{"key1"}, "checksum1"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_tileset_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result.empty());
}

// Tests for all_checksums_match method

TEST_F(ArtifactChecksumProviderTest, AllChecksumsMatch_AllMatch_ReturnsTrue)
{
    // Setup: All checksums match between current and cached
    std::vector<ArtifactKey> artifact_keys = {ArtifactKey{"key1"}, ArtifactKey{"key2"}, ArtifactKey{"key3"}};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}, {ArtifactKey{"key3"}, "checksum3"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}, {ArtifactKey{"key3"}, "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    bool result = provider_.all_checksums_tileset_match(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ArtifactChecksumProviderTest, AllChecksumsMatch_SomeDoNotMatch_ReturnsFalse)
{
    // Setup: Some checksums don't match between current and cached
    std::vector<ArtifactKey> artifact_keys = {ArtifactKey{"key1"}, ArtifactKey{"key2"}, ArtifactKey{"key3"}};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {
        {ArtifactKey{"key1"}, "checksum1"},
        {ArtifactKey{"key2"}, "checksum2_modified"},
        {ArtifactKey{"key3"}, "checksum3"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {
        {ArtifactKey{"key1"}, "checksum1"}, {ArtifactKey{"key2"}, "checksum2"}, {ArtifactKey{"key3"}, "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    bool result = provider_.all_checksums_tileset_match(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactChecksumProviderTest, AllChecksumsMatch_EmptyArtifactKeysList_ReturnsTrue)
{
    // Setup: Empty artifact keys list - vacuous truth, all of nothing match
    std::vector<ArtifactKey> artifact_keys = {};
    std::unordered_map<ArtifactKey, std::string> current_checksums = {{ArtifactKey{"key1"}, "checksum1"}};
    std::unordered_map<ArtifactKey, std::string> cached_checksums = {{ArtifactKey{"key1"}, "checksum1"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    bool result = provider_.all_checksums_tileset_match(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result); // Empty list means all (none) match
}

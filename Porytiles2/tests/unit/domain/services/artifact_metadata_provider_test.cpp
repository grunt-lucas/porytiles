#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <chrono>
#include <string>
#include <unordered_map>

#include "porytiles2/domain/services/artifact_metadata_provider.hpp"

using namespace porytiles2;
using namespace ::testing;

class MockArtifactMetadataProvider : public ArtifactMetadataProvider {
  public:
    MOCK_METHOD((std::vector<std::string>), get_porytiles_artifact_keys, (const std::string &tileset_name),
                (const, override));
    MOCK_METHOD((std::vector<std::string>), get_porymap_artifact_keys, (const std::string &tileset_name),
                (const, override));
    MOCK_METHOD((std::unordered_map<std::string, std::string>), compute_artifact_checksums,
                (const std::string &tileset_name), (const, override));
    MOCK_METHOD((std::unordered_map<std::string, std::string>), load_cached_checksums,
                (const std::string &tileset_name), (const, override));
    MOCK_METHOD((Result<void>), cache_checksums,
                ((const std::string &tileset_name), (const std::unordered_map<std::string, std::string> &checksums)),
                (const, override));
    MOCK_METHOD((std::unordered_map<std::string, Timestamp>), get_porymap_timestamps, (const std::string &tileset_name),
                (const, override));
    MOCK_METHOD((std::unordered_map<std::string, Timestamp>), get_porytiles_timestamps,
                (const std::string &tileset_name), (const, override));
};

class ArtifactMetadataProviderTest : public ::testing::Test {
  protected:
    MockArtifactMetadataProvider provider_;
    std::string test_tileset_name_ = "test_tileset";
};

TEST_F(ArtifactMetadataProviderTest, ArePorytilesAssetsNewer_BothEmpty_ReturnsFalse) {
    // Setup: Both timestamp maps are empty
    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_))
        .WillOnce(Return(std::unordered_map<std::string, Timestamp>{}));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_))
        .WillOnce(Return(std::unordered_map<std::string, Timestamp>{}));

    // Act
    bool result = provider_.are_porytiles_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorytilesAssetsNewer_PorytilesEmpty_ReturnsFalse) {
    // Setup: Porytiles timestamps empty, Porymap has timestamps
    auto now = std::filesystem::file_time_type::clock::now();
    std::unordered_map<std::string, Timestamp> porymap_timestamps = {{"file1", now}};

    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_)).WillOnce(Return(porymap_timestamps));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_))
        .WillOnce(Return(std::unordered_map<std::string, Timestamp>{}));

    // Act
    bool result = provider_.are_porytiles_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorytilesAssetsNewer_PorymapEmpty_ReturnsFalse) {
    // Setup: Porymap timestamps empty, Porytiles has timestamps
    auto now = std::filesystem::file_time_type::clock::now();
    std::unordered_map<std::string, Timestamp> porytiles_timestamps = {{"file1", now}};

    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_))
        .WillOnce(Return(std::unordered_map<std::string, Timestamp>{}));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_)).WillOnce(Return(porytiles_timestamps));

    // Act
    bool result = provider_.are_porytiles_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorytilesAssetsNewer_AllPorytilesNewer_ReturnsTrue) {
    // Setup: All Porytiles files are newer than all Porymap files
    auto base_time = std::filesystem::file_time_type::clock::now();
    auto older_time = base_time - std::chrono::hours(2);
    auto newer_time = base_time + std::chrono::hours(2);

    std::unordered_map<std::string, Timestamp> porymap_timestamps = {{"porymap1", older_time},
                                                                     {"porymap2", older_time - std::chrono::hours(1)}};

    std::unordered_map<std::string, Timestamp> porytiles_timestamps = {
        {"porytiles1", newer_time}, {"porytiles2", newer_time + std::chrono::hours(1)}};

    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_)).WillOnce(Return(porymap_timestamps));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_)).WillOnce(Return(porytiles_timestamps));

    // Act
    bool result = provider_.are_porytiles_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorytilesAssetsNewer_SomePorymapNewer_ReturnsFalse) {
    // Setup: Some Porymap files are newer than some Porytiles files
    auto base_time = std::filesystem::file_time_type::clock::now();

    std::unordered_map<std::string, Timestamp> porymap_timestamps = {
        {"porymap_old", base_time - std::chrono::hours(3)},
        {"porymap_new", base_time + std::chrono::hours(1)} // This is newer than oldest Porytiles
    };

    std::unordered_map<std::string, Timestamp> porytiles_timestamps = {
        {"porytiles_old", base_time - std::chrono::hours(1)}, // Oldest Porytiles
        {"porytiles_new", base_time + std::chrono::hours(3)}};

    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_)).WillOnce(Return(porymap_timestamps));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_)).WillOnce(Return(porytiles_timestamps));

    // Act
    bool result = provider_.are_porytiles_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorymapAssetsNewer_BothEmpty_ReturnsFalse) {
    // Setup: Both timestamp maps are empty
    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_))
        .WillOnce(Return(std::unordered_map<std::string, Timestamp>{}));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_))
        .WillOnce(Return(std::unordered_map<std::string, Timestamp>{}));

    // Act
    bool result = provider_.are_porymap_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorymapAssetsNewer_AllPorymapNewer_ReturnsTrue) {
    // Setup: All Porymap files are newer than all Porytiles files
    auto base_time = std::filesystem::file_time_type::clock::now();
    auto older_time = base_time - std::chrono::hours(2);
    auto newer_time = base_time + std::chrono::hours(2);

    std::unordered_map<std::string, Timestamp> porytiles_timestamps = {
        {"porytiles1", older_time}, {"porytiles2", older_time - std::chrono::hours(1)}};

    std::unordered_map<std::string, Timestamp> porymap_timestamps = {{"porymap1", newer_time},
                                                                     {"porymap2", newer_time + std::chrono::hours(1)}};

    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_)).WillOnce(Return(porymap_timestamps));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_)).WillOnce(Return(porytiles_timestamps));

    // Act
    bool result = provider_.are_porymap_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorymapAssetsNewer_SomePorytilesNewer_ReturnsFalse) {
    // Setup: Some Porytiles files are newer than some Porymap files
    auto base_time = std::filesystem::file_time_type::clock::now();

    std::unordered_map<std::string, Timestamp> porytiles_timestamps = {
        {"porytiles_old", base_time - std::chrono::hours(3)},
        {"porytiles_new", base_time + std::chrono::hours(1)} // This is newer than oldest Porymap
    };

    std::unordered_map<std::string, Timestamp> porymap_timestamps = {
        {"porymap_old", base_time - std::chrono::hours(1)}, // Oldest Porymap
        {"porymap_new", base_time + std::chrono::hours(3)}};

    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_)).WillOnce(Return(porymap_timestamps));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_)).WillOnce(Return(porytiles_timestamps));

    // Act
    bool result = provider_.are_porymap_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactMetadataProviderTest, ArePorymapAssetsNewer_SingleFileEach_CorrectComparison) {
    // Setup: Single file in each set
    auto base_time = std::filesystem::file_time_type::clock::now();
    auto porymap_time = base_time + std::chrono::hours(1);
    auto porytiles_time = base_time;

    std::unordered_map<std::string, Timestamp> porymap_timestamps = {{"porymap_file", porymap_time}};
    std::unordered_map<std::string, Timestamp> porytiles_timestamps = {{"porytiles_file", porytiles_time}};

    EXPECT_CALL(provider_, get_porymap_timestamps(test_tileset_name_)).WillOnce(Return(porymap_timestamps));
    EXPECT_CALL(provider_, get_porytiles_timestamps(test_tileset_name_)).WillOnce(Return(porytiles_timestamps));

    // Act
    bool result = provider_.are_porymap_assets_newer(test_tileset_name_);

    // Assert
    EXPECT_TRUE(result); // Porymap file is newer
}

// Tests for find_unsynced_artifacts method

TEST_F(ArtifactMetadataProviderTest, FindUnsyncedArtifacts_AllChecksumsMatch_ReturnsEmpty) {
    // Setup: All checksums match between current and cached
    std::vector<std::string> artifact_keys = {"key1", "key2", "key3"};
    std::unordered_map<std::string, std::string> current_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2"}, {"key3", "checksum3"}};
    std::unordered_map<std::string, std::string> cached_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2"}, {"key3", "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result.empty());
}

TEST_F(ArtifactMetadataProviderTest, FindUnsyncedArtifacts_SomeChecksumsDoNotMatch_ReturnsUnsyncedKeys) {
    // Setup: Some checksums don't match between current and cached
    std::vector<std::string> artifact_keys = {"key1", "key2", "key3"};
    std::unordered_map<std::string, std::string> current_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2_modified"}, {"key3", "checksum3"}};
    std::unordered_map<std::string, std::string> cached_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2"}, {"key3", "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "key2");
}

TEST_F(ArtifactMetadataProviderTest, FindUnsyncedArtifacts_AllChecksumsDoNotMatch_ReturnsAllKeys) {
    // Setup: All checksums don't match between current and cached
    std::vector<std::string> artifact_keys = {"key1", "key2", "key3"};
    std::unordered_map<std::string, std::string> current_checksums = {
        {"key1", "checksum1_modified"}, {"key2", "checksum2_modified"}, {"key3", "checksum3_modified"}};
    std::unordered_map<std::string, std::string> cached_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2"}, {"key3", "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 3);
    EXPECT_THAT(result, UnorderedElementsAre("key1", "key2", "key3"));
}

TEST_F(ArtifactMetadataProviderTest, FindUnsyncedArtifacts_MissingCurrentChecksum_ReturnsUnsyncedKey) {
    // Setup: Key is missing from current checksums but present in cached
    std::vector<std::string> artifact_keys = {"key1", "key2"};
    std::unordered_map<std::string, std::string> current_checksums = {{"key1", "checksum1"}}; // key2 missing
    std::unordered_map<std::string, std::string> cached_checksums = {{"key1", "checksum1"}, {"key2", "checksum2"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "key2");
}

TEST_F(ArtifactMetadataProviderTest, FindUnsyncedArtifacts_MissingCachedChecksum_ReturnsUnsyncedKey) {
    // Setup: Key is missing from cached checksums but present in current
    std::vector<std::string> artifact_keys = {"key1", "key2"};
    std::unordered_map<std::string, std::string> current_checksums = {{"key1", "checksum1"}, {"key2", "checksum2"}};
    std::unordered_map<std::string, std::string> cached_checksums = {{"key1", "checksum1"}}; // key2 missing

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "key2");
}

TEST_F(ArtifactMetadataProviderTest, FindUnsyncedArtifacts_EmptyArtifactKeysList_ReturnsEmpty) {
    // Setup: Empty artifact keys list
    std::vector<std::string> artifact_keys = {};
    std::unordered_map<std::string, std::string> current_checksums = {{"key1", "checksum1"}};
    std::unordered_map<std::string, std::string> cached_checksums = {{"key1", "checksum1"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    auto result = provider_.find_unsynced_artifacts(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result.empty());
}

// Tests for all_checksums_match method

TEST_F(ArtifactMetadataProviderTest, AllChecksumsMatch_AllMatch_ReturnsTrue) {
    // Setup: All checksums match between current and cached
    std::vector<std::string> artifact_keys = {"key1", "key2", "key3"};
    std::unordered_map<std::string, std::string> current_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2"}, {"key3", "checksum3"}};
    std::unordered_map<std::string, std::string> cached_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2"}, {"key3", "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    bool result = provider_.all_checksums_match(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ArtifactMetadataProviderTest, AllChecksumsMatch_SomeDoNotMatch_ReturnsFalse) {
    // Setup: Some checksums don't match between current and cached
    std::vector<std::string> artifact_keys = {"key1", "key2", "key3"};
    std::unordered_map<std::string, std::string> current_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2_modified"}, {"key3", "checksum3"}};
    std::unordered_map<std::string, std::string> cached_checksums = {
        {"key1", "checksum1"}, {"key2", "checksum2"}, {"key3", "checksum3"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    bool result = provider_.all_checksums_match(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ArtifactMetadataProviderTest, AllChecksumsMatch_EmptyArtifactKeysList_ReturnsTrue) {
    // Setup: Empty artifact keys list - vacuous truth, all of nothing match
    std::vector<std::string> artifact_keys = {};
    std::unordered_map<std::string, std::string> current_checksums = {{"key1", "checksum1"}};
    std::unordered_map<std::string, std::string> cached_checksums = {{"key1", "checksum1"}};

    EXPECT_CALL(provider_, compute_artifact_checksums(test_tileset_name_)).WillOnce(Return(current_checksums));
    EXPECT_CALL(provider_, load_cached_checksums(test_tileset_name_)).WillOnce(Return(cached_checksums));

    // Act
    bool result = provider_.all_checksums_match(test_tileset_name_, artifact_keys);

    // Assert
    EXPECT_TRUE(result); // Empty list means all (none) match
}

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
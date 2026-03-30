#include "gtest/gtest.h"

#include <filesystem>

#include "porytiles2/utilities/filesystem_utils.hpp"

using namespace porytiles2;

class FilesystemUtilsTest : public ::testing::Test {};

TEST_F(FilesystemUtilsTest, StripAllExtensions_SingleExtension)
{
    EXPECT_EQ(strip_all_extensions("tiles.png"), std::filesystem::path{"tiles"});
    EXPECT_EQ(strip_all_extensions("image.jpg"), std::filesystem::path{"image"});
}

TEST_F(FilesystemUtilsTest, StripAllExtensions_MultipleExtensions)
{
    EXPECT_EQ(strip_all_extensions("tiles.4bpp.smol"), std::filesystem::path{"tiles"});
    EXPECT_EQ(strip_all_extensions("archive.tar.gz"), std::filesystem::path{"archive"});
    EXPECT_EQ(strip_all_extensions("file.a.b.c.d"), std::filesystem::path{"file"});
}

TEST_F(FilesystemUtilsTest, StripAllExtensions_NoExtension)
{
    EXPECT_EQ(strip_all_extensions("tiles"), std::filesystem::path{"tiles"});
    EXPECT_EQ(strip_all_extensions("README"), std::filesystem::path{"README"});
}

TEST_F(FilesystemUtilsTest, StripAllExtensions_DotFile)
{
    // Dot files should be unchanged - the leading dot is part of the filename, not an extension
    EXPECT_EQ(strip_all_extensions(".gitignore"), std::filesystem::path{".gitignore"});
    EXPECT_EQ(strip_all_extensions(".bashrc"), std::filesystem::path{".bashrc"});
}

TEST_F(FilesystemUtilsTest, StripAllExtensions_DotFileWithExtension)
{
    // Dot files with actual extensions should have extensions stripped
    EXPECT_EQ(strip_all_extensions(".config.json"), std::filesystem::path{".config"});
    EXPECT_EQ(strip_all_extensions(".data.tar.gz"), std::filesystem::path{".data"});
}

TEST_F(FilesystemUtilsTest, StripAllExtensions_PathWithDirectory)
{
    EXPECT_EQ(strip_all_extensions("data/tiles.4bpp.smol"), std::filesystem::path{"data/tiles"});
    EXPECT_EQ(strip_all_extensions("foo/bar/baz.txt"), std::filesystem::path{"foo/bar/baz"});
    EXPECT_EQ(strip_all_extensions("/absolute/path/file.png"), std::filesystem::path{"/absolute/path/file"});
}

TEST_F(FilesystemUtilsTest, StripAllExtensions_EmptyPath)
{
    EXPECT_EQ(strip_all_extensions(""), std::filesystem::path{""});
}

TEST_F(FilesystemUtilsTest, StripAllExtensions_DirectoryOnly)
{
    // Just a directory with trailing slash behavior
    EXPECT_EQ(strip_all_extensions("data/"), std::filesystem::path{"data/"});
}

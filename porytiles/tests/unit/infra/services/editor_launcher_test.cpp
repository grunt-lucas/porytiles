#include "gtest/gtest.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>

#include "porytiles/infra/services/editor_launcher.hpp"

using namespace porytiles;

namespace {

const std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "porytiles_test_editor_launcher";

class EditorLauncherTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        save_and_unset("PORYTILES_EDITOR");
        save_and_unset("VISUAL");
        save_and_unset("EDITOR");
        save("PATH");
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override
    {
        for (const auto &[name, value] : saved_env_) {
            if (value.has_value()) {
                setenv(name.c_str(), value->c_str(), 1);
            }
            else {
                unsetenv(name.c_str());
            }
        }
        std::filesystem::remove_all(test_dir);
    }

    void save(const std::string &name)
    {
        const char *value = std::getenv(name.c_str());
        saved_env_[name] = value == nullptr ? std::nullopt : std::optional<std::string>{value};
    }

    void save_and_unset(const std::string &name)
    {
        save(name);
        unsetenv(name.c_str());
    }

    // Writes an executable shell script that records its arguments into out_file.
    static std::filesystem::path write_recorder_script(const std::string &name, const std::filesystem::path &out_file)
    {
        const auto script_path = test_dir / name;
        {
            std::ofstream script{script_path};
            script << "#!/bin/sh\n";
            script << "printf '%s' \"$1\" > \"" << out_file.string() << "\"\n";
        }
        std::filesystem::permissions(
            script_path,
            std::filesystem::perms::owner_all | std::filesystem::perms::group_read |
                std::filesystem::perms::others_read);
        return script_path;
    }

    static void write_executable(const std::filesystem::path &path)
    {
        {
            std::ofstream file{path};
            file << "#!/bin/sh\n";
        }
        std::filesystem::permissions(path, std::filesystem::perms::owner_all);
    }

    EditorLauncher launcher_{};

  private:
    std::map<std::string, std::optional<std::string>> saved_env_;
};

TEST_F(EditorLauncherTest, ResolvePrefersPorytilesEditor)
{
    setenv("PORYTILES_EDITOR", "porytiles-editor", 1);
    setenv("VISUAL", "visual-editor", 1);
    setenv("EDITOR", "plain-editor", 1);

    const auto result = launcher_.resolve_editor_command();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "porytiles-editor");
}

TEST_F(EditorLauncherTest, ResolvePrefersVisualOverEditor)
{
    setenv("VISUAL", "visual-editor", 1);
    setenv("EDITOR", "plain-editor", 1);

    const auto result = launcher_.resolve_editor_command();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "visual-editor");
}

TEST_F(EditorLauncherTest, ResolveUsesEditor)
{
    setenv("EDITOR", "plain-editor", 1);

    const auto result = launcher_.resolve_editor_command();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "plain-editor");
}

TEST_F(EditorLauncherTest, ResolveSkipsEmptyValues)
{
    setenv("PORYTILES_EDITOR", "", 1);
    setenv("VISUAL", "", 1);
    setenv("EDITOR", "plain-editor", 1);

    const auto result = launcher_.resolve_editor_command();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "plain-editor");
}

TEST_F(EditorLauncherTest, ResolveFallsBackToPathSearch)
{
    write_executable(test_dir / "vim");
    write_executable(test_dir / "vi");
    setenv("PATH", test_dir.string().c_str(), 1);

    const auto result = launcher_.resolve_editor_command();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "vim");
}

TEST_F(EditorLauncherTest, ResolveFallbackOrderPrefersNano)
{
    write_executable(test_dir / "nano");
    write_executable(test_dir / "vim");
    write_executable(test_dir / "vi");
    setenv("PATH", test_dir.string().c_str(), 1);

    const auto result = launcher_.resolve_editor_command();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "nano");
}

TEST_F(EditorLauncherTest, ResolveFallbackSkipsEmptyPathComponents)
{
    write_executable(test_dir / "nano");
    const std::string path_value = "::" + test_dir.string();
    setenv("PATH", path_value.c_str(), 1);

    const auto result = launcher_.resolve_editor_command();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "nano");
}

TEST_F(EditorLauncherTest, ResolveFailsWhenPathUnset)
{
    unsetenv("PATH");

    const auto result = launcher_.resolve_editor_command();

    EXPECT_FALSE(result.has_value());
}

TEST_F(EditorLauncherTest, ResolveFailsWhenNoEditorAvailable)
{
    const auto empty_dir = test_dir / "empty";
    std::filesystem::create_directories(empty_dir);
    setenv("PATH", empty_dir.string().c_str(), 1);

    const auto result = launcher_.resolve_editor_command();

    EXPECT_FALSE(result.has_value());
}

TEST_F(EditorLauncherTest, EditFileInvokesEditorWithPath)
{
    const auto out_file = test_dir / "recorded_args.txt";
    const auto script_path = write_recorder_script("recorder.sh", out_file);
    setenv("PORYTILES_EDITOR", script_path.string().c_str(), 1);

    // Spaces and a single quote in the target exercise the shell quoting.
    const auto target = test_dir / "name with 'quote'.yaml";

    const auto result = launcher_.edit_file(target);

    ASSERT_TRUE(result.has_value());
    std::ifstream recorded{out_file};
    std::stringstream contents;
    contents << recorded.rdbuf();
    EXPECT_EQ(contents.str(), target.string());
}

TEST_F(EditorLauncherTest, EditFileSucceedsOnZeroExit)
{
    setenv("PORYTILES_EDITOR", "true", 1);

    const auto result = launcher_.edit_file(test_dir / "config.yaml");

    EXPECT_TRUE(result.has_value());
}

TEST_F(EditorLauncherTest, EditFileFailsOnNonZeroExit)
{
    setenv("PORYTILES_EDITOR", "false", 1);

    const auto result = launcher_.edit_file(test_dir / "config.yaml");

    EXPECT_FALSE(result.has_value());
}

TEST_F(EditorLauncherTest, CreateAndEditFileCreatesMissingDirectoriesAndFile)
{
    setenv("PORYTILES_EDITOR", "true", 1);
    const auto target = test_dir / "porytiles" / "tilesets" / "gTileset_Test" / "config.yaml";

    const auto result = launcher_.create_and_edit_file(target);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(std::filesystem::is_regular_file(target));
    EXPECT_EQ(std::filesystem::file_size(target), 0U);
}

TEST_F(EditorLauncherTest, CreateAndEditFileFailsWhenDirectoryBlockedByFile)
{
    setenv("PORYTILES_EDITOR", "true", 1);
    // A regular file where a parent directory should go makes create_directories fail.
    {
        const std::ofstream blocker{test_dir / "blocker"};
    }
    const auto target = test_dir / "blocker" / "config.yaml";

    const auto result = launcher_.create_and_edit_file(target);

    EXPECT_FALSE(result.has_value());
}

TEST_F(EditorLauncherTest, CreateAndEditFileKeepsExistingContent)
{
    setenv("PORYTILES_EDITOR", "true", 1);
    const auto target = test_dir / "config.yaml";
    {
        std::ofstream out{target};
        out << "fieldmap:\n";
    }

    const auto result = launcher_.create_and_edit_file(target);

    EXPECT_TRUE(result.has_value());
    std::ifstream in{target};
    std::stringstream contents;
    contents << in.rdbuf();
    EXPECT_EQ(contents.str(), "fieldmap:\n");
}

} // namespace

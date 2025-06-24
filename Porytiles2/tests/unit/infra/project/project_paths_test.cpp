#include "gtest/gtest.h"

#include "porytiles2/infra/project/project_paths.hpp"

using namespace porytiles;

TEST(ProjectPathsTests, BehaviorsHeaderDefaultShouldWork) {
    const ProjectPaths paths{"/foo/bar"};
    EXPECT_EQ(paths.BehaviorsHeader(), "/foo/bar/include/constants/metatile_behaviors.h");
}

TEST(ProjectPathsTests, BehaviorsHeaderWithConfigShouldWork) {
    ProjectPaths paths1{"/foo/bar"};
    paths1.SetBehaviorsHeaderOverridePath("src");
    EXPECT_EQ(paths1.BehaviorsHeader(), "/foo/bar/src/metatile_behaviors.h");

    ProjectPaths paths2{"/foo/bar"};
    paths2.SetBehaviorsHeaderOverrideFile("my_cool_header.h");
    EXPECT_EQ(paths2.BehaviorsHeader(), "/foo/bar/include/constants/my_cool_header.h");

    ProjectPaths paths3{"/foo/bar"};
    paths3.SetBehaviorsHeaderOverridePath("src");
    paths3.SetBehaviorsHeaderOverrideFile("my_cool_header.h");
    EXPECT_EQ(paths3.BehaviorsHeader(), "/foo/bar/src/my_cool_header.h");
}
#include "gtest/gtest.h"

#include "porytiles2/infra/project/ProjectPaths.hpp"

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

TEST(ProjectPathsTests, PrimaryTilesetDirectoryShouldWork) {
  const ProjectPaths paths{"/foo/bar"};
  EXPECT_EQ(paths.PrimaryTilesetDirectory("general"), "/foo/bar/data/tilesets/primary/general");
  EXPECT_EQ(paths.PrimaryTilesetDirectory("building"), "/foo/bar/data/tilesets/primary/building");
}

TEST(ProjectPathsTests, PrimaryTilesetPngFilesShouldWork) {
  const ProjectPaths paths{"/foo/bar"};

  EXPECT_EQ(paths.PrimaryBottomPng("general"),
            "/foo/bar/data/tilesets/primary/general/porytiles/bottom.png");
  EXPECT_EQ(paths.PrimaryMiddlePng("general"),
            "/foo/bar/data/tilesets/primary/general/porytiles/middle.png");
  EXPECT_EQ(paths.PrimaryTopPng("general"),
            "/foo/bar/data/tilesets/primary/general/porytiles/top.png");

  EXPECT_EQ(paths.PrimaryBottomPng("building"),
            "/foo/bar/data/tilesets/primary/building/porytiles/bottom.png");
  EXPECT_EQ(paths.PrimaryMiddlePng("building"),
            "/foo/bar/data/tilesets/primary/building/porytiles/middle.png");
  EXPECT_EQ(paths.PrimaryTopPng("building"),
            "/foo/bar/data/tilesets/primary/building/porytiles/top.png");
}

TEST(ProjectPathsTests, SecondaryTilesetDirectoryShouldWork) {
  const ProjectPaths paths{"/foo/bar"};
  EXPECT_EQ(paths.SecondaryTilesetDirectory("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods");
  EXPECT_EQ(paths.SecondaryTilesetDirectory("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104");
}

TEST(ProjectPathsTests, SecondaryTilesetPngFilesShouldWork) {
  const ProjectPaths paths{"/foo/bar"};

  EXPECT_EQ(paths.SecondaryBottomPng("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods/porytiles/bottom.png");
  EXPECT_EQ(paths.SecondaryMiddlePng("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods/porytiles/middle.png");
  EXPECT_EQ(paths.SecondaryTopPng("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods/porytiles/top.png");

  EXPECT_EQ(paths.SecondaryBottomPng("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104/porytiles/bottom.png");
  EXPECT_EQ(paths.SecondaryMiddlePng("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104/porytiles/middle.png");
  EXPECT_EQ(paths.SecondaryTopPng("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104/porytiles/top.png");
}

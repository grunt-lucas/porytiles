#include "gtest/gtest.h"

#include "porytiles2/infra/project/project_paths.hpp"

using namespace porytiles2;

TEST(ProjectPathsTests, BehaviorsHeaderDefaultShouldWork) {
  const ProjectPaths paths{"/foo/bar"};
  EXPECT_EQ(paths.behaviors_header(), "/foo/bar/include/constants/metatile_behaviors.h");
}

TEST(ProjectPathsTests, BehaviorsHeaderWithConfigShouldWork) {
  ProjectPaths paths1{"/foo/bar"};
  paths1.set_behaviors_header_override_path("src");
  EXPECT_EQ(paths1.behaviors_header(), "/foo/bar/src/metatile_behaviors.h");

  ProjectPaths paths2{"/foo/bar"};
  paths2.set_behaviors_header_override_file("my_cool_header.h");
  EXPECT_EQ(paths2.behaviors_header(), "/foo/bar/include/constants/my_cool_header.h");

  ProjectPaths paths3{"/foo/bar"};
  paths3.set_behaviors_header_override_path("src");
  paths3.set_behaviors_header_override_file("my_cool_header.h");
  EXPECT_EQ(paths3.behaviors_header(), "/foo/bar/src/my_cool_header.h");
}

TEST(ProjectPathsTests, PrimaryTilesetDirectoryShouldWork) {
  const ProjectPaths paths{"/foo/bar"};
  EXPECT_EQ(paths.primary_tileset_directory("general"), "/foo/bar/data/tilesets/primary/general");
  EXPECT_EQ(paths.primary_tileset_directory("building"), "/foo/bar/data/tilesets/primary/building");
}

TEST(ProjectPathsTests, PrimaryTilesetPngFilesShouldWork) {
  const ProjectPaths paths{"/foo/bar"};

  EXPECT_EQ(paths.primary_bottom_png("general"),
            "/foo/bar/data/tilesets/primary/general/porytiles/bottom.png");
  EXPECT_EQ(paths.primary_middle_png("general"),
            "/foo/bar/data/tilesets/primary/general/porytiles/middle.png");
  EXPECT_EQ(paths.primary_top_png("general"),
            "/foo/bar/data/tilesets/primary/general/porytiles/top.png");

  EXPECT_EQ(paths.primary_bottom_png("building"),
            "/foo/bar/data/tilesets/primary/building/porytiles/bottom.png");
  EXPECT_EQ(paths.primary_middle_png("building"),
            "/foo/bar/data/tilesets/primary/building/porytiles/middle.png");
  EXPECT_EQ(paths.primary_top_png("building"),
            "/foo/bar/data/tilesets/primary/building/porytiles/top.png");
}

TEST(ProjectPathsTests, SecondaryTilesetDirectoryShouldWork) {
  const ProjectPaths paths{"/foo/bar"};
  EXPECT_EQ(paths.secondary_tileset_directory("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods");
  EXPECT_EQ(paths.secondary_tileset_directory("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104");
}

TEST(ProjectPathsTests, SecondaryTilesetPngFilesShouldWork) {
  const ProjectPaths paths{"/foo/bar"};

  EXPECT_EQ(paths.secondary_bottom_png("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods/porytiles/bottom.png");
  EXPECT_EQ(paths.secondary_middle_png("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods/porytiles/middle.png");
  EXPECT_EQ(paths.secondary_top_png("petalburg_woods"),
            "/foo/bar/data/tilesets/secondary/petalburg_woods/porytiles/top.png");

  EXPECT_EQ(paths.secondary_bottom_png("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104/porytiles/bottom.png");
  EXPECT_EQ(paths.secondary_middle_png("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104/porytiles/middle.png");
  EXPECT_EQ(paths.secondary_top_png("route_104"),
            "/foo/bar/data/tilesets/secondary/route_104/porytiles/top.png");
}

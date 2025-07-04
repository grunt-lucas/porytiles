#include "gtest/gtest.h"

#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"
#include "porytiles2/domain/repos/RgbaImageRepo.hpp"
#include "porytiles2/infra/repos/PngRgbaImageRepo.hpp"

using namespace porytiles;

TEST(PngRgbaImageRepoTests, OpenShouldFailGracefullyOnBadFile) {
  const std::unique_ptr<RgbaImageRepo> importer = std::make_unique<PngRgbaImageRepo>();

  auto result = importer->Read("Resources/Tests/Unit/png/non_existent_file.png");
  ASSERT_FALSE(result.has_value());
  EXPECT_TRUE(result.error().contains(
      "Failed to open file 'Resources/Tests/Unit/png/non_existent_file.png'"));

  auto result2 = importer->Read("Resources/Tests/Unit/metatile_behaviors.h");
  ASSERT_FALSE(result2.has_value());
  EXPECT_TRUE(result2.error().contains("Failed to recognize format of file "
                                       "'Resources/Tests/Unit/metatile_behaviors.h'"));
}
